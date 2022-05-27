@cd /d "%~dp0"
@regsvr32.exe BassAudioSource.ax /u /s
@if %errorlevel% NEQ 0 goto error
:success
@echo.
@echo.
@echo    Uninstallation succeeded.
@echo.
@goto done
:error
@echo.
@echo.
@echo    Uninstallation failed.
@echo.
@echo    You need to right click "Uninstall_BassAudioSource_32.cmd" and choose "run as admin".
@echo.
:done
@pause >NUL
