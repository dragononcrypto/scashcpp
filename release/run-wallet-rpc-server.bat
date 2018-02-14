REM We don't want to override system QT paths for other applications but we don't want to use some random version too.
REM So set required one for our session only:
set QT_QPA_PLATFORM_PLUGIN_PATH=%~dp0/plugins

cd %~dp0

start ./scash-qt.exe -server=1
