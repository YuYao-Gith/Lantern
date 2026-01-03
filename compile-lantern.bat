@echo off
set "JAVAFX_LIB=C:\jfx-2501\lib"
cd /d "%~dp0"
javac --module-path "%JAVAFX_LIB%" --add-modules javafx.controls,javafx.fxml,javafx.web Lantern.java
