@echo off
chcp 65001 > nul
echo ========================================================
echo   NẠP FIRMWARE ESP32-S3 (ĐÃ FIX LỖI LAG & RỚT BLUETOOTH)
echo ========================================================
echo.
echo Đang tự động tìm cổng COM và nạp firmware...
"%USERPROFILE%\.platformio\penv\Scripts\platformio.exe" run -e esp32s3 -t upload
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================================
    echo   ✅ NẠP THÀNH CÔNG 100%!
    echo   ESP32 đã được cập nhật firmware tối ưu tốc độ 50Hz.
    echo ========================================================
) else (
    echo.
    echo ========================================================
    echo   ❌ KHÔNG TÌM THẤY ESP32!
    echo   Hãy cắm cáp USB nối ESP32 với máy tính rồi chạy lại.
    echo ========================================================
)
pause
