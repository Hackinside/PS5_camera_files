@echo off
uv run --with pyvirtualcam --with opencv-python --with cv2-enumerate-cameras python "%~dp0ps5_camera.py" %*
exit /b %errorlevel%
