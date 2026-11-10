import argparse
import json
import os
import time
from functools import lru_cache
from pathlib import Path

import cv2
import numpy as np
import pyvirtualcam
from cv2_enumerate_cameras import enumerate_cameras

TITLE = "PS5 HD Camera"
CONTROLS = "PS5 camera controls - W balances a centered neutral card, Q exits"
PROFILE = Path(os.environ["LOCALAPPDATA"]) / "PS5StereoCamera" / "balance.json"


def balance_profile():
    try:
        gains = np.asarray(json.loads(PROFILE.read_text()), dtype=float)
        if gains.shape == (3,) and np.all((gains >= 0.5) & (gains <= 2)):
            return gains
    except (ValueError, OSError, TypeError):
        pass
    return np.ones(3)


def calibrate(frame):
    h, w = frame.shape[:2]
    roi = frame[h//2-40:h//2+40, w//2-40:w//2+40]
    values = np.median(roi, axis=(0, 1))
    if values.min() < 25 or values.max() > 235 or np.std(roi, axis=(0, 1)).max() > 18:
        return None
    gains = values.mean() / values
    if np.any((gains < 0.5) | (gains > 2)):
        return None
    PROFILE.parent.mkdir(parents=True, exist_ok=True)
    temp = PROFILE.with_suffix(".tmp")
    temp.write_text(json.dumps(gains.tolist()))
    temp.replace(PROFILE)
    return gains


@lru_cache(maxsize=64)
def tone_curve(shadows, midtones, highlights):
    # Each slider sets the output luminance at a fixed input level.
    levels = np.array([0, shadows, midtones + 64, highlights + 160, 255])
    return np.interp(np.arange(256), [0, 32, 128, 208, 255], levels).round().astype(np.uint8)


def meter_midtones(image, setting, target):
    # Meter the central subject after processing, including the current curve.
    h, w = image.shape[:2]
    center = image[h//4:3*h//4:8, w//3:2*w//3:8]
    gray = cv2.cvtColor(center, cv2.COLOR_BGR2GRAY)
    subject, highlight = np.percentile(gray, [65, 98])
    error = target - subject
    if abs(error) <= 5 or (error > 0 and highlight >= 245):
        return setting
    step = int(np.clip(round(error / 8), -3, 3))
    return int(np.clip(setting + step, 0, 96))


def process(frame, previous, strength, gains, curve=None):
    # Suppress chroma speckle without smoothing luminance edges.
    if strength:
        yuv = cv2.cvtColor(frame, cv2.COLOR_BGR2YCrCb)
        for channel in (1, 2):
            yuv[:, :, channel] = cv2.addWeighted(yuv[:, :, channel], 1-strength/8,
                cv2.medianBlur(yuv[:, :, channel], 3), strength/8, 0)
        frame = cv2.cvtColor(yuv, cv2.COLOR_YCrCb2BGR)
        if previous is not None:
            b, g, r = cv2.split(cv2.absdiff(frame, previous))
            delta = cv2.max(cv2.max(b, g), r)
            mixed = cv2.addWeighted(frame, 1-strength/16, previous, strength/16, 0)
            cv2.copyTo(mixed, cv2.compare(delta, 12, cv2.CMP_LT), frame)
    table = np.clip(np.arange(256)[:, None] * gains, 0, 255).astype(np.uint8)
    balanced = cv2.LUT(frame, table.reshape(256, 1, 3))
    if curve is not None:
        yuv = cv2.cvtColor(balanced, cv2.COLOR_BGR2YCrCb)
        yuv[:, :, 0] = cv2.LUT(yuv[:, :, 0], curve)
        balanced = cv2.cvtColor(yuv, cv2.COLOR_YCrCb2BGR)
    return balanced, frame


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("view", nargs="?", default="left", choices=["left", "right", "stereo", "test"])
    parser.add_argument("--gain", type=int, choices=range(9), default=4)
    args = parser.parse_args()
    test = args.view == "test"
    settings = {"Gain": (args.gain, 8), "Auto gain": (0, 1), "Saturation": (4, 8), "Denoise": (5, 8)}
    settings.update({"Shadows (32 neutral)": (46, 64), "Midtones (64 neutral)": (92, 96),
                     "Highlights (48 neutral)": (52, 95), "Auto midtones": (1, 1),
                     "Subject target": (105, 200)})
    gains = balance_profile()
    if not test:
        cv2.namedWindow(CONTROLS, cv2.WINDOW_NORMAL)
        cv2.resizeWindow(CONTROLS, 720, 300)
        for name, (initial, limit) in settings.items():
            cv2.createTrackbar(name, CONTROLS, initial, limit, lambda _: None)
        cv2.namedWindow(TITLE, cv2.WINDOW_NORMAL if args.view == "stereo" else cv2.WINDOW_AUTOSIZE)
        if args.view == "stereo":
            cv2.resizeWindow(TITLE, 1280, 360)
    virtual = None
    camera = None
    original = {}
    previous = {}
    frames = 0
    next_meter = 0.0
    status = "W = balance on a neutral card"
    try:
        while True:
            key = cv2.waitKey(5) & 255 if not test else 255
            if not test and (key in (27, ord("q")) or cv2.getWindowProperty(TITLE, cv2.WND_PROP_VISIBLE) < 1):
                return
            if camera is None:
                device = next((d for d in enumerate_cameras(cv2.CAP_DSHOW)
                               if (d.vid, d.pid) == (0x05A9, 0x058C)), None)
                if device:
                    camera = cv2.VideoCapture(device.index, cv2.CAP_DSHOW)
                    if not camera.isOpened():
                        camera.release()
                        camera = None
                if camera is None:
                    if test:
                        raise SystemExit("Camera unavailable")
                    cv2.setWindowTitle(TITLE, "Waiting for PS5 HD Camera")
                    time.sleep(0.5)
                    continue
                camera.set(cv2.CAP_PROP_FRAME_WIDTH, 3840)
                camera.set(cv2.CAP_PROP_FRAME_HEIGHT, 1080)
                controls = {"Gain": cv2.CAP_PROP_GAIN, "Saturation": cv2.CAP_PROP_SATURATION}
                original = {prop: camera.get(prop) for prop in controls.values()}
                applied = {}
                previous.clear()
            current = {name: val if test else cv2.getTrackbarPos(name, CONTROLS)
                       for name, (val, _) in settings.items()}
            for name, prop in controls.items():
                if applied.get(name) != current[name]:
                    if not camera.set(prop, current[name]):
                        status = f"Camera rejected {name}"
                    applied[name] = current[name]
            ok, frame = camera.read()
            if not ok or frame.shape[:2] != (1080, 3840):
                camera.release()
                camera = None
                if test:
                    raise SystemExit("Stereo stream failed")
                if virtual:
                    virtual.send(np.zeros((1080, 1920, 3), np.uint8))
                continue
            eyes = {"left": frame[:, :1920], "right": frame[:, 1920:]}
            primary = "right" if args.view == "right" else "left"
            if current["Auto gain"] and frames % 20 == 0:
                sample = eyes[primary][180:900:8, 320:1600:8]
                luma = cv2.cvtColor(sample, cv2.COLOR_BGR2GRAY)
                median, highlight = np.percentile(luma, [50, 98])
                step = -1 if highlight > 245 or median > 130 else int(median < 75 and highlight < 225)
                cv2.setTrackbarPos("Gain", CONTROLS, int(np.clip(current["Gain"] + step, 0, 8)))
            if key == ord("w"):
                new = calibrate(eyes[primary])
                if new is None:
                    status = "Calibration rejected: center a uniformly lit neutral card"
                else:
                    gains = new
                    status = "Neutral balance saved"
            output = {}
            curve = tone_curve(current["Shadows (32 neutral)"], current["Midtones (64 neutral)"],
                               current["Highlights (48 neutral)"])
            for side in eyes if args.view in ("stereo", "test") else (primary,):
                output[side], previous[side] = process(eyes[side], previous.get(side), current["Denoise"], gains, curve)
            frames += 1
            now = time.monotonic()
            if not test and current["Auto midtones"] and now >= next_meter:
                name = "Midtones (64 neutral)"
                cv2.setTrackbarPos(name, CONTROLS, meter_midtones(
                    output[primary], current[name], current["Subject target"]))
                next_meter = now + 0.5
            if test:
                if frames >= 15:
                    for side, image in output.items():
                        sat = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)[:, :, 1].mean()
                        print(f"{side} mean saturation: {sat:.2f}")
                        if sat < 1:
                            raise SystemExit("No measurable color")
                    return
                continue
            if virtual is None and frames % 30 == 1:
                try:
                    virtual = pyvirtualcam.Camera(1920, 1080, 30, fmt=pyvirtualcam.PixelFormat.BGR, backend="obs")
                except RuntimeError:
                    status = "OBS Virtual Camera unavailable - retrying"
            if virtual:
                virtual.send(output[primary])
            preview = np.hstack((output["left"], output["right"])) if args.view == "stereo" else output[primary]
            cv2.imshow(TITLE, preview)
            cv2.setWindowTitle(TITLE, f"{TITLE} | {status}")
    finally:
        if virtual:
            virtual.close()
        if camera:
            for prop, value in original.items():
                if value >= 0:
                    camera.set(prop, value)
            camera.release()
        cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
