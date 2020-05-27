/*
 * Qt Installer script for a non-interactive installation of Qt5 on Windows.
 *
 * Run with:
 * qt-unified-windows-x86-online.exe --verbose --script qt-installer-windows.qs
 *
 * globals QInstaller, QMessageBox, buttons, gui, installer, console
 */

function Controller() {
    installer.setMessageBoxAutomaticAnswer("installationError", QMessageBox.Retry);
    installer.setMessageBoxAutomaticAnswer("installationErrorWithRetry", QMessageBox.Retry);
    installer.setMessageBoxAutomaticAnswer("DownloadError", QMessageBox.Retry);
    installer.setMessageBoxAutomaticAnswer("archiveDownloadError", QMessageBox.Retry);

    // Continue on installation to an existing (possibly empty) directory.
    installer.setMessageBoxAutomaticAnswer("OverwriteTargetDirectory", QMessageBox.Yes);

    // Continue at the end of the installation
    installer.installationFinished.connect(function() {
        console.log("Step: InstallationFinished");
        gui.clickButton(buttons.NextButton);
    });
}


Controller.prototype.WelcomePageCallback = function()
{
    console.log("Step: " + gui.currentPageWidget());
    // At least for 3.0.4 immediately clicking Next fails, so wait a bit.
    // https://github.com/benlau/qtci/commit/85cb986b66af4807a928c70e13d82d00dc26ebf0
    gui.clickButton(buttons.NextButton, 1000);
};

Controller.prototype.CredentialsPageCallback = function()
{
    console.log("Step: " + gui.currentPageWidget());

    var page = gui.pageWidgetByObjectName("CredentialsPage");
    var username = installer.environmentVariable("QT_ACCOUNT_USERNAME");
    var password = installer.environmentVariable("QT_ACCOUNT_PASSWORD");
    page.loginWidget.EmailLineEdit.setText(username);
    page.loginWidget.PasswordLineEdit.setText(password);

    gui.clickButton(buttons.NextButton);
};

Controller.prototype.IntroductionPageCallback = function()
{
    console.log("Step: " + gui.currentPageWidget());
    gui.clickButton(buttons.NextButton);
};

Controller.prototype.ObligationsPageCallback = function()
{
    console.log("Step: " + gui.currentPageWidget());

    var page = gui.pageWidgetByObjectName("ObligationsPage");
    page.obligationsAgreement.setChecked(true);
    page.completeChanged();
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.DynamicTelemetryPluginFormCallback = function()
{
    console.log("Step: " + gui.currentPageWidget());

    var page = gui.pageWidgetByObjectName("DynamicTelemetryPluginForm");
    page.statisticGroupBox.disableStatisticRadioButton.setChecked(true);
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.TargetDirectoryPageCallback = function()
{
    console.log("Step: " + gui.currentPageWidget());
    gui.clickButton(buttons.NextButton);
};

Controller.prototype.ComponentSelectionPageCallback = function() {
    console.log("Step: " + gui.currentPageWidget());

    var selection = gui.pageWidgetByObjectName("ComponentSelectionPage");
    gui.findChild(selection, "Latest releases").checked = false;
    gui.findChild(selection, "LTS").checked = true;
    gui.findChild(selection, "FetchCategoryButton").click();

    // Look for Name elements in
    // https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt5_5123/Updates.xml
    var widget = gui.currentPageWidget();
    widget.deselectAll();
    widget.selectComponent("qt.qt5.5123.win32_msvc2017");
    widget.selectComponent("qt.qt5.5123.win64_msvc2017_64");

    gui.clickButton(buttons.NextButton);
}

Controller.prototype.LicenseAgreementPageCallback = function()
{
    console.log("Step: " + gui.currentPageWidget());

    gui.currentPageWidget().AcceptLicenseRadioButton.setChecked(true);
    gui.clickButton(buttons.NextButton);
};

Controller.prototype.StartMenuDirectoryPageCallback = function()
{
    console.log("Step: " + gui.currentPageWidget());
    gui.clickButton(buttons.NextButton);
};

Controller.prototype.ReadyForInstallationPageCallback = function()
{
    console.log("Step: " + gui.currentPageWidget());
    gui.clickButton(buttons.NextButton);
};

Controller.prototype.FinishedPageCallback = function()
{
    console.log("Step: " + gui.currentPageWidget());

    var checkBoxForm = gui.currentPageWidget().LaunchQtCreatorCheckBoxForm;
    if (checkBoxForm && checkBoxForm.launchQtCreatorCheckBox) {
        checkBoxForm.launchQtCreatorCheckBox.checked = false;
    }
    gui.clickButton(buttons.FinishButton);
};
