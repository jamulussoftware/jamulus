/******************************************************************************\
 * Copyright (c) 2004-2025
 *
 * Author(s):
 *  Volker Fischer
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#include "util.h"
#if defined( JAMULUS_USE_JUCE_NET )
#include "juce_net_abstraction.h"
#endif
#if !defined( HEADLESS ) && !defined( JAMULUS_USE_JUCE_NET )
#include <QCoreApplication>
#endif
#include <QFile>
#ifdef _WIN32
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    include <arpa/inet.h>
#    include <sys/socket.h>
#    include <netinet/in.h>
#    include <unistd.h>
#    include <netdb.h>
#endif
#include <juce_core/juce_core.h>

/* Implementation *************************************************************/
// Input level meter implementation --------------------------------------------
void CStereoSignalLevelMeter::Update ( const CVector<short>& vecsAudio, const int iMonoBlockSizeSam, const bool bIsStereoIn )
{
    // Get maximum of current block
    //
    // Speed optimization:
    // - we only make use of the negative values and ignore the positive ones (since
    //   int16 has range {-32768, 32767}) -> we do not need to call the fabs() function
    // - we only evaluate every third sample
    //
    // With these speed optimizations we might loose some information in
    // special cases but for the average music signals the following code
    // should give good results.
    short sMinLOrMono = 0;
    short sMinR       = 0;

    if ( bIsStereoIn )
    {
        // stereo in
        for ( int i = 0; i < 2 * iMonoBlockSizeSam; i += 6 ) // 2 * 3 = 6 -> stereo
        {
            // left (or mono) and right channel
            sMinLOrMono = std::min ( sMinLOrMono, vecsAudio[i] );
            sMinR       = std::min ( sMinR, vecsAudio[i + 1] );
        }

        // in case of mono out use minimum of both channels
        if ( !bIsStereoOut )
        {
            sMinLOrMono = std::min ( sMinLOrMono, sMinR );
        }
    }
    else
    {
        // mono in
        for ( int i = 0; i < iMonoBlockSizeSam; i += 3 )
        {
            sMinLOrMono = std::min ( sMinLOrMono, vecsAudio[i] );
        }
    }

    // apply smoothing, if in stereo out mode, do this for two channels
    dCurLevelLOrMono = UpdateCurLevel ( dCurLevelLOrMono, -sMinLOrMono );

    if ( bIsStereoOut )
    {
        dCurLevelR = UpdateCurLevel ( dCurLevelR, -sMinR );
    }
}

double CStereoSignalLevelMeter::UpdateCurLevel ( double dCurLevel, const double dMax )
{
    // decrease max with time
    if ( dCurLevel >= METER_FLY_BACK )
    {
        dCurLevel *= dSmoothingFactor;
    }
    else
    {
        dCurLevel = 0;
    }

    // update current level -> only use maximum
    if ( dMax > dCurLevel )
    {
        return dMax;
    }
    else
    {
        return dCurLevel;
    }
}

double CStereoSignalLevelMeter::CalcLogResultForMeter ( const double& dLinearLevel )
{
    const double dNormLevel = dLinearLevel / _MAXSHORT;

    // logarithmic measure
    double dLevelForMeterdB = -100000.0; // large negative value

    if ( dNormLevel > 0 )
    {
        dLevelForMeterdB = 20.0 * log10 ( dNormLevel );
    }

    // map to signal level meter (linear transformation of the input
    // level range to the level meter range)
    dLevelForMeterdB -= LOW_BOUND_SIG_METER;
    dLevelForMeterdB *= NUM_STEPS_LED_BAR / ( UPPER_BOUND_SIG_METER - LOW_BOUND_SIG_METER );

    if ( dLevelForMeterdB < 0 )
    {
        dLevelForMeterdB = 0;
    }

    return dLevelForMeterdB;
}

// CRC -------------------------------------------------------------------------
void CCRC::Reset()
{
    // init state shift-register with ones. Set all registers to "1" with
    // bit-wise not operation
    iStateShiftReg = ~uint32_t ( 0 );
}

void CCRC::AddByte ( const uint8_t byNewInput )
{
    for ( int i = 0; i < 8; i++ )
    {
        // shift bits in shift-register for transition
        iStateShiftReg <<= 1;

        // take bit, which was shifted out of the register-size and place it
        // at the beginning (LSB)
        // (If condition is not satisfied, implicitly a "0" is added)
        if ( ( iStateShiftReg & iBitOutMask ) > 0 )
        {
            iStateShiftReg |= 1;
        }

        // add new data bit to the LSB
        if ( ( byNewInput & ( 1 << ( 8 - i - 1 ) ) ) > 0 )
        {
            iStateShiftReg ^= 1;
        }

        // add mask to shift-register if first bit is true
        if ( iStateShiftReg & 1 )
        {
            iStateShiftReg ^= iPoly;
        }
    }
}

uint32_t CCRC::GetCRC()
{
    // return inverted shift-register (1's complement)
    iStateShiftReg = ~iStateShiftReg;

    // remove bit which where shifted out of the shift-register frame
    return iStateShiftReg & ( iBitOutMask - 1 );
}

// CHighPrecisionTimer implementation ******************************************
CHighPrecisionTimer::CHighPrecisionTimer ( const bool bNewUseDoubleSystemFrameSize ) :
    veciTimeOutIntervals(),
    iCurPosInVector ( 0 ),
    iIntervalCounter ( 0 ),
    bUseDoubleSystemFrameSize ( bNewUseDoubleSystemFrameSize )
{
#    if ( SYSTEM_FRAME_SIZE_SAMPLES != 64 ) && ( DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES != 128 )
#        error "Only system frame size of 64 and 128 samples is supported by this module"
#    endif
#    if ( SYSTEM_SAMPLE_RATE_HZ != 48000 )
#        error "Only a system sample rate of 48 kHz is supported by this module"
#    endif

    veciTimeOutIntervals.Init ( 3 );
    veciTimeOutIntervals[0] = 0;
    veciTimeOutIntervals[1] = 1;
    veciTimeOutIntervals[2] = 0;
}

void CHighPrecisionTimer::Start()
{
    iCurPosInVector  = 0;
    iIntervalCounter = 0;

    if ( bUseDoubleSystemFrameSize )
    {
        juce::HighResolutionTimer::startTimer ( 2 );
    }
    else
    {
        juce::HighResolutionTimer::startTimer ( 1 );
    }
}

void CHighPrecisionTimer::Stop()
{
    juce::HighResolutionTimer::stopTimer();
}

void CHighPrecisionTimer::hiResTimerCallback()
{
    if ( veciTimeOutIntervals[iCurPosInVector] == iIntervalCounter )
    {
        iIntervalCounter = 0;

        iCurPosInVector++;
        if ( iCurPosInVector == veciTimeOutIntervals.Size() )
        {
            iCurPosInVector = 0;
        }

        if ( callback )
        {
            callback();
        }
    }
    else
    {
        iIntervalCounter++;
    }
}

/******************************************************************************\
* GUI Utilities                                                                *
\******************************************************************************/
#if !defined( HEADLESS )
#if defined( JAMULUS_USE_JUCE_NET )
CBaseDlg::CBaseDlg ( const juce::String& title, juce::Component* parent ) : juce::DialogWindow ( title, juce::Colours::darkgrey, true, true )
{
    setUsingNativeTitleBar ( true );
    setResizable ( false, false );

    if ( parent != nullptr )
    {
        centreAroundComponent ( parent, getWidth() > 0 ? getWidth() : 420, getHeight() > 0 ? getHeight() : 240 );
    }
}

int CBaseDlg::exec()
{
    if ( getWidth() <= 0 || getHeight() <= 0 )
    {
        centreWithSize ( 420, 240 );
    }
    else
    {
        centreWithSize ( getWidth(), getHeight() );
    }
    setVisible ( true );
    return 0;
}

void CBaseDlg::closeButtonPressed()
{
    exitModalState ( 0 );
    setVisible ( false );
}

bool CBaseDlg::keyPressed ( const juce::KeyPress& key )
{
    if ( key == juce::KeyPress::escapeKey )
    {
        return true;
    }
    return juce::DialogWindow::keyPressed ( key );
}

CAboutDlg::CAboutDlg() : CBaseDlg ( juce::String ( "About " ) + juce::String ( APP_NAME ) )
{
}

int CAboutDlg::exec()
{
    const juce::String title   = juce::String ( "About " ) + juce::String ( APP_NAME );
    const juce::String message = juce::String ( GetVersionAndNameStr().toStdString() );
    juce::AlertWindow::showMessageBoxAsync ( juce::AlertWindow::InfoIcon, title, message );
    return 0;
}

namespace
{
class LicenceContent : public juce::Component
{
public:
    LicenceContent ( std::function<void()> onAcceptIn, std::function<void()> onDeclineIn ) :
        onAccept ( std::move ( onAcceptIn ) ),
        onDecline ( std::move ( onDeclineIn ) )
    {
        addAndMakeVisible ( lblLicence );
        lblLicence.setText ( "This server requires you accept conditions before you can join. Please read these in the chat window.",
                             juce::dontSendNotification );
        lblLicence.setJustificationType ( juce::Justification::topLeft );

        addAndMakeVisible ( chbAgree );
        chbAgree.setButtonText ( "I have read the conditions and agree." );

        addAndMakeVisible ( butAccept );
        butAccept.setButtonText ( "Accept" );
        butAccept.setEnabled ( false );

        addAndMakeVisible ( butDecline );
        butDecline.setButtonText ( "Decline" );

        chbAgree.onClick = [this] { butAccept.setEnabled ( chbAgree.getToggleState() ); };
        butAccept.onClick = [this]
        {
            if ( onAccept )
            {
                onAccept();
            }
        };
        butDecline.onClick = [this]
        {
            if ( onDecline )
            {
                onDecline();
            }
        };
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced ( 12 );
        lblLicence.setBounds ( area.removeFromTop ( 48 ) );
        area.removeFromTop ( 8 );
        chbAgree.setBounds ( area.removeFromTop ( 24 ) );
        area.removeFromTop ( 8 );
        auto buttons = area.removeFromTop ( 28 );
        auto declineWidth = 90;
        auto acceptWidth  = 90;
        butDecline.setBounds ( buttons.removeFromRight ( declineWidth ) );
        buttons.removeFromRight ( 8 );
        butAccept.setBounds ( buttons.removeFromRight ( acceptWidth ) );
    }

private:
    juce::Label           lblLicence;
    juce::ToggleButton    chbAgree;
    juce::TextButton      butAccept;
    juce::TextButton      butDecline;
    std::function<void()> onAccept;
    std::function<void()> onDecline;
};
}

CLicenceDlg::CLicenceDlg() : CBaseDlg ( juce::String ( APP_NAME ) )
{
    auto onAccept = [this]
    {
        exitModalState ( 1 );
        setVisible ( false );
    };
    auto onDecline = [this]
    {
        exitModalState ( 0 );
        setVisible ( false );
    };
    setContentOwned ( new LicenceContent ( onAccept, onDecline ), true );
    setSize ( 520, 180 );
}

CHelpMenu::CHelpMenu ( const bool bIsClient, juce::Component* parentIn ) :
    parent ( parentIn ),
    AboutDlg()
{
    if ( bIsClient )
    {
        menu.addItem ( "Getting Started...", [this] { OnHelpClientGetStarted(); } );
        menu.addItem ( "Software Manual...", [this] { OnHelpSoftwareMan(); } );
    }
    else
    {
        menu.addItem ( "Getting Started...", [this] { OnHelpServerGetStarted(); } );
    }
    menu.addSeparator();
    menu.addItem ( "What's This", [this] { OnHelpWhatsThis(); } );
    menu.addSeparator();
    menu.addItem ( "About Jamulus...", [this] { OnHelpAbout(); } );
    menu.addItem ( "About Qt...", [this] { OnHelpAboutQt(); } );
}

void CHelpMenu::show()
{
    juce::PopupMenu::Options options;
    if ( parent != nullptr )
    {
        options = options.withTargetComponent ( parent );
    }
    menu.showMenuAsync ( options );
}

juce::PopupMenu& CHelpMenu::getMenu()
{
    return menu;
}

void CHelpMenu::OnHelpWhatsThis()
{
    juce::AlertWindow::showMessageBoxAsync ( juce::AlertWindow::InfoIcon, "Help", "What's This mode is not available." );
}

void CHelpMenu::OnHelpAbout()
{
    AboutDlg.exec();
}

void CHelpMenu::OnHelpAboutQt()
{
    juce::AlertWindow::showMessageBoxAsync ( juce::AlertWindow::InfoIcon, "About Qt", "Qt information is not available." );
}

void CHelpMenu::OnHelpClientGetStarted()
{
    juce::URL ( CLIENT_GETTING_STARTED_URL ).launchInDefaultBrowser();
}

void CHelpMenu::OnHelpServerGetStarted()
{
    juce::URL ( SERVER_GETTING_STARTED_URL ).launchInDefaultBrowser();
}

void CHelpMenu::OnHelpSoftwareMan()
{
    juce::URL ( SOFTWARE_MANUAL_URL ).launchInDefaultBrowser();
}

CLanguageComboBox::CLanguageComboBox()
{
    onChange = [this] { OnLanguageActivated(); };
}

void CLanguageComboBox::setLanguageChangedCallback ( std::function<void ( QString )> callback )
{
    onLanguageChanged = std::move ( callback );
}

void CLanguageComboBox::Init ( QString& strSelLanguage )
{
    const QMap<QString, QString> TranslMap = CLocale::GetAvailableTranslations();
    clear ( juce::dontSendNotification );
    languageCodes.clear();

    int iCnt                  = 0;
    int iIdxOfEnglishLanguage = 0;
    iIdxSelectedLanguage      = INVALID_INDEX;

    for ( auto it = TranslMap.constBegin(); it != TranslMap.constEnd(); ++it )
    {
        const QString langCode = it.key();
        const QString label    = langCode; // avoid QLocale in JUCE build

        addItem ( label.toStdString(), iCnt + 1 );
        languageCodes.push_back ( langCode );

        if ( langCode.compare ( "en" ) == 0 )
        {
            iIdxOfEnglishLanguage = iCnt;
        }

        if ( langCode.compare ( strSelLanguage ) == 0 )
        {
            iIdxSelectedLanguage = iCnt;
        }

        iCnt++;
    }

    if ( iIdxSelectedLanguage == INVALID_INDEX )
    {
        strSelLanguage       = "en";
        iIdxSelectedLanguage = iIdxOfEnglishLanguage;
    }

    setSelectedItemIndex ( iIdxSelectedLanguage, juce::dontSendNotification );
}

void CLanguageComboBox::OnLanguageActivated()
{
    const int iLanguageIdx = getSelectedItemIndex();
    if ( iLanguageIdx < 0 || iLanguageIdx >= static_cast<int> ( languageCodes.size() ) )
    {
        return;
    }

    if ( iIdxSelectedLanguage != iLanguageIdx )
    {
        juce::AlertWindow::showMessageBoxAsync ( juce::AlertWindow::InfoIcon,
                                                 "Restart Required",
                                                 "Please restart the application for the language change to take effect." );

        if ( onLanguageChanged )
        {
            onLanguageChanged ( languageCodes[iLanguageIdx] );
        }
    }
}

void CMinimumStackedLayout::addComponent ( juce::Component* component )
{
    if ( component == nullptr )
    {
        return;
    }

    components.push_back ( component );
    addAndMakeVisible ( component );
    component->setVisible ( false );

    if ( currentIndex == -1 )
    {
        setCurrentIndex ( 0 );
    }
}

void CMinimumStackedLayout::setCurrentIndex ( int index )
{
    if ( index < 0 || index >= static_cast<int> ( components.size() ) )
    {
        return;
    }

    if ( currentIndex >= 0 && currentIndex < static_cast<int> ( components.size() ) )
    {
        components[currentIndex]->setVisible ( false );
    }

    currentIndex = index;

    if ( auto* current = getCurrentComponent() )
    {
        current->setVisible ( true );
    }

    resized();
}

juce::Component* CMinimumStackedLayout::getCurrentComponent() const
{
    if ( currentIndex < 0 || currentIndex >= static_cast<int> ( components.size() ) )
    {
        return nullptr;
    }

    return components[currentIndex];
}

void CMinimumStackedLayout::resized()
{
    if ( auto* current = getCurrentComponent() )
    {
        current->setBounds ( getLocalBounds() );
    }
}
#else
// About dialog ----------------------------------------------------------------
CAboutDlg::CAboutDlg ( QWidget* parent ) : CBaseDlg ( parent )
{
    setupUi ( this );

    // general description of software
    txvAbout->setText ( "<p>" +
                        tr ( "This app enables musicians to perform real-time jam sessions "
                             "over the internet." ) +
                        "<br>" +
                        tr ( "There is a server which collects "
                             " the audio data from each client, mixes the audio data and sends the mix "
                             " back to each client." ) +
                        "</p>"
                        "<p><font face=\"courier\">" // GPL header text
                        "This program is free software; you can redistribute it and/or modify "
                        "it under the terms of the GNU General Public License as published by "
                        "the Free Software Foundation; either version 2 of the License, or "
                        "(at your option) any later version.<br>This program is distributed in "
                        "the hope that it will be useful, but WITHOUT ANY WARRANTY; without "
                        "even the implied warranty of MERCHANTABILITY or FITNESS FOR A "
                        "PARTICULAR PURPOSE. See the GNU General Public License for more "
                        "details.<br>You should have received a copy of the GNU General Public "
                        "License along with his program; if not, write to the Free Software "
                        "Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 "
                        "USA"
                        "</font></p>" );

    // libraries used by this compilation
    txvLibraries->setText ( tr ( "This app uses the following libraries, resources or code snippets:" ) + "<p>" +
                            tr ( "Qt cross-platform application framework" ) + QString ( " %1 " ).arg ( QT_VERSION_STR ) + tr ( "(build)" ) +
                            QString ( ", %1 " ).arg ( qVersion() ) + tr ( "(runtime)" ) +
                            ", <i><a href=\"https://www.qt.io\">https://www.qt.io</a></i>"
                            "</p>"
                            "<p>"
                            "Opus Interactive Audio Codec"
                            ", <i><a href=\"https://www.opus-codec.org\">https://www.opus-codec.org</a></i>"
                            "</p>"
#    ifndef SERVER_ONLY
#        if defined( _WIN32 ) && !defined( WITH_JACK )
                            "<p>"
                            "ASIO (Audio Stream I/O) SDK"
                            ", <i><a href=\"https://www.steinberg.net/developers/\">https://www.steinberg.net/developers</a></i>"
                            "<br>"
                            "ASIO is a trademark and software of Steinberg Media Technologies GmbH"
                            "</p>"
#        endif
#        ifndef HEADLESS
                            "<p>" +
                            tr ( "Audio reverberation code by Perry R. Cook and Gary P. Scavone" ) +
                            ", 1995 - 2021"
                            ", The Synthesis ToolKit in C++ (STK)"
                            ", <i><a href=\"https://ccrma.stanford.edu/software/stk\">https://ccrma.stanford.edu/software/stk</a></i>"
                            "</p>"
                            "<p>" +
                            QString ( tr ( "Some pixmaps are from the %1" ) ).arg ( "Open Clip Art Library (OCAL)" ) +
                            ", <i><a href=\"https://openclipart.org\">https://openclipart.org</a></i>"
                            "</p>"
                            "<p>" +
                            tr ( "Flag icons by Mark James" ) +
                            ", <i><a href=\"http://www.famfamfam.com\">http://www.famfamfam.com</a></i>"
                            "</p>"
                            "<p>" +
                            QString ( tr ( "Some sound samples are from %1" ) ).arg ( "Freesound" ) +
                            ", <i><a href=\"https://freesound.org\">https://freesound.org</a></i>"
                            "</p>"
#        endif
#    endif
    );

    // contributors list
    txvContributors->setText (
        "<p>Volker Fischer (<a href=\"https://github.com/corrados\">corrados</a>)</p>"
        "<p>Peter L. Jones (<a href=\"https://github.com/pljones\">pljones</a>)</p>"
        "<p>Jonathan Baker-Bates (<a href=\"https://github.com/gilgongo\">gilgongo</a>)</p>"
        "<p>ann0see (<a href=\"https://github.com/ann0see\">ann0see</a>)</p>"
        "<p>Daniele Masato (<a href=\"https://github.com/doloopuntil\">doloopuntil</a>)</p>"
        "<p>Martin Schilde (<a href=\"https://github.com/geheimerEichkater\">geheimerEichkater</a>)</p>"
        "<p>Simon Tomlinson (<a href=\"https://github.com/sthenos\">sthenos</a>)</p>"
        "<p>Marc jr. Landolt (<a href=\"https://github.com/braindef\">braindef</a>)</p>"
        "<p>Olivier Humbert (<a href=\"https://github.com/trebmuh\">trebmuh</a>)</p>"
        "<p>Tarmo Johannes (<a href=\"https://github.com/tarmoj\">tarmoj</a>)</p>"
        "<p>mirabilos (<a href=\"https://github.com/mirabilos\">mirabilos</a>)</p>"
        "<p>Hector Martin (<a href=\"https://github.com/marcan\">marcan</a>)</p>"
        "<p>newlaurent62 (<a href=\"https://github.com/newlaurent62\">newlaurent62</a>)</p>"
        "<p>AronVietti (<a href=\"https://github.com/AronVietti\">AronVietti</a>)</p>"
        "<p>Emlyn Bolton (<a href=\"https://github.com/emlynmac\">emlynmac</a>)</p>"
        "<p>Jos van den Oever (<a href=\"https://github.com/vandenoever\">vandenoever</a>)</p>"
        "<p>Tormod Volden (<a href=\"https://github.com/tormodvolden\">tormodvolden</a>)</p>"
        "<p>Alberstein8 (<a href=\"https://github.com/Alberstein8\">Alberstein8</a>)</p>"
        "<p>Gauthier Fleutot Östervall (<a href=\"https://github.com/fleutot\">fleutot</a>)</p>"
        "<p>Tony Mountifield (<a href=\"https://github.com/softins\">softins</a>)</p>"
        "<p>HPS (<a href=\"https://github.com/hselasky\">hselasky</a>)</p>"
        "<p>Stanislas Michalak (<a href=\"https://github.com/stanislas-m\">stanislas-m</a>)</p>"
        "<p>JP Cimalando (<a href=\"https://github.com/jpcima\">jpcima</a>)</p>"
        "<p>Adam Sampson (<a href=\"https://github.com/atsampson\">atsampson</a>)</p>"
        "<p>Jakob Jarmar (<a href=\"https://github.com/jarmar\">jarmar</a>)</p>"
        "<p>Stefan Weil (<a href=\"https://github.com/stweil\">stweil</a>)</p>"
        "<p>Nils Brederlow (<a href=\"https://github.com/dingodoppelt\">dingodoppelt</a>)</p>"
        "<p>Sebastian Krzyszkowiak (<a href=\"https://github.com/dos1\">dos1</a>)</p>"
        "<p>Bryan Flamig (<a href=\"https://github.com/bflamig\">bflamig</a>)</p>"
        "<p>Kris Raney (<a href=\"https://github.com/kraney\">kraney</a>)</p>"
        "<p>dszgit (<a href=\"https://github.com/dszgit\">dszgit</a>)</p>"
        "<p>nefarius2001 (<a href=\"https://github.com/nefarius2001\">nefarius2001</a>)</p>"
        "<p>jc-Rosichini (<a href=\"https://github.com/jc-Rosichini\">jc-Rosichini</a>)</p>"
        "<p>Julian Santander (<a href=\"https://github.com/j-santander\">j-santander</a>)</p>"
        "<p>chigkim (<a href=\"https://github.com/chigkim\">chigkim</a>)</p>"
        "<p>Bodo (<a href=\"https://github.com/bomm\">bomm</a>)</p>"
        "<p>Christian Hoffmann (<a href=\"https://github.com/hoffie\">hoffie</a>)</p>"
        "<p>jp8 (<a href=\"https://github.com/jp8\">jp8</a>)</p>"
        "<p>James (<a href=\"https://github.com/jdrage\">jdrage</a>)</p>"
        "<p>ranfdev (<a href=\"https://github.com/ranfdev\">ranfdev</a>)</p>"
        "<p>bspeer (<a href=\"https://github.com/bspeer\">bspeer</a>)</p>"
        "<p>Martin Passing (<a href=\"https://github.com/passing\">passing</a>)</p>"
        "<p>DonC (<a href=\"https://github.com/dcorson-ticino-com\">dcorson-ticino-com</a>)</p>"
        "<p>David Kastrup (<a href=\"https://github.com/dakhubgit\">dakhubgit</a>)</p>"
        "<p>Jordan Lum (<a href=\"https://github.com/mulyaj\">mulyaj</a>)</p>"
        "<p>Noam Postavsky (<a href=\"https://github.com/npostavs\">npostavs</a>)</p>"
        "<p>David Savinkoff (<a href=\"https://github.com/DavidSavinkoff\">DavidSavinkoff</a>)</p>"
        "<p>Johannes Brauers (<a href=\"https://github.com/JohannesBrx\">JohannesBrx</a>)</p>"
        "<p>Henk De Groot (<a href=\"https://github.com/henkdegroot\">henkdegroot</a>)</p>"
        "<p>Ferenc Wágner (<a href=\"https://github.com/wferi\">wferi</a>)</p>"
        "<p>Martin Kaistra (<a href=\"https://github.com/djfun\">djfun</a>)</p>"
        "<p>Burkhard Volkemer (<a href=\"https://github.com/buv\">buv</a>)</p>"
        "<p>Magnus Groß (<a href=\"https://github.com/vimpostor\">vimpostor</a>)</p>"
        "<p>Julien Taverna (<a href=\"https://github.com/jujudusud\">jujudusud</a>)</p>"
        "<p>Detlef Hennings (<a href=\"https://github.com/DetlefHennings\">DetlefHennings</a>)</p>"
        "<p>drummer1154 (<a href=\"https://github.com/drummer1154\">drummer1154</a>)</p>"
        "<p>helgeerbe (<a href=\"https://github.com/helgeerbe\">helgeerbe</a>)</p>"
        "<p>Hk1020 (<a href=\"https://github.com/Hk1020\">Hk1020</a>)</p>"
        "<p>Jeroen van Veldhuizen (<a href=\"https://github.com/jeroenvv\">jeroenvv</a>)</p>"
        "<p>Reinhard (<a href=\"https://github.com/reinhardwh\">reinhardwh</a>)</p>"
        "<p>Stefan Menzel (<a href=\"https://github.com/menzels\">menzels</a>)</p>"
        "<p>Dau Huy Ngoc (<a href=\"https://github.com/ngocdh\">ngocdh</a>)</p>"
        "<p>Jiri Popek (<a href=\"https://github.com/jardous\">jardous</a>)</p>"
        "<p>Gary Wang (<a href=\"https://github.com/BLumia\">BLumia</a>)</p>"
        "<p>RobyDati (<a href=\"https://github.com/RobyDati\">RobyDati</a>)</p>"
        "<p>Rob-NY (<a href=\"https://github.com/Rob-NY\">Rob-NY</a>)</p>"
        "<p>Thai Pangsakulyanont (<a href=\"https://github.com/dtinth\">dtinth</a>)</p>"
        "<p>Peter Goderie (<a href=\"https://github.com/pgScorpio\">pgScorpio</a>)</p>"
        "<p>Dan Garton (<a href=\"https://github.com/danryu\">danryu</a>)</p>"
        "<br>" +
        tr ( "For details on the contributions check out the %1" )
            .arg ( "<a href=\"https://github.com/jamulussoftware/jamulus/graphs/contributors\">" + tr ( "Github Contributors list" ) + "</a>." ) );

    // translators
    txvTranslation->setText ( "<p><b>" + tr ( "Spanish" ) +
                              "</b></p>"
                              "<p>Daryl Hanlon (<a href=\"https://github.com/ignotus666\">ignotus666</a>)</p>"
                              "<p><b>" +
                              tr ( "French" ) +
                              "</b></p>"
                              "<p>Olivier Humbert (<a href=\"https://github.com/trebmuh\">trebmuh</a>)</p>"
                              "<p>Julien Taverna (<a href=\"https://github.com/jujudusud\">jujudusud</a>)</p>"
                              "<p><b>" +
                              tr ( "Portuguese" ) +
                              "</b></p>"
                              "<p>Miguel de Matos (<a href=\"https://github.com/Snayler\">Snayler</a>)</p>"
                              "<p>Melcon Moraes (<a href=\"https://github.com/melcon\">melcon</a>)</p>"
                              "<p>Manuela Silva (<a href=\"https://hosted.weblate.org/user/mansil/\">mansil</a>)</p>"
                              "<p>gbonaspetti (<a href=\"https://hosted.weblate.org/user/gbonaspetti/\">gbonaspetti</a>)</p>"
                              "<p><b>" +
                              tr ( "Dutch" ) +
                              "</b></p>"
                              "<p>Jeroen Geertzen (<a href=\"https://github.com/jerogee\">jerogee</a>)</p>"
                              "<p>Henk De Groot (<a href=\"https://github.com/henkdegroot\">henkdegroot</a>)</p>"
                              "<p><b>" +
                              tr ( "Italian" ) +
                              "</b></p>"
                              "<p>Giuseppe Sapienza (<a href=\"https://github.com/dzpex\">dzpex</a>)</p>"
                              "<p><b>" +
                              tr ( "German" ) +
                              "</b></p>"
                              "<p>Volker Fischer (<a href=\"https://github.com/corrados\">corrados</a>)</p>"
                              "<p>Roland Moschel (<a href=\"https://github.com/rolamos\">rolamos</a>)</p>"
                              "<p><b>" +
                              tr ( "Polish" ) +
                              "</b></p>"
                              "<p>Martyna Danysz (<a href=\"https://github.com/Martyna27\">Martyna27</a>)</p>"
                              "<p>Tomasz Bojczuk (<a href=\"https://github.com/SeeLook\">SeeLook</a>)</p>"
                              "<p><b>" +
                              tr ( "Swedish" ) +
                              "</b></p>"
                              "<p>Daniel (<a href=\"https://github.com/genesisproject2020\">genesisproject2020</a>)</p>"
                              "<p>tygyh (<a href=\"https://github.com/tygyh\">tygyh</a>)</p>"
                              "<p>Allan Nordhøy (<a href=\"https://hosted.weblate.org/user/kingu/\">kingu</a>)</p>"
                              "<p><b>" +
                              tr ( "Korean" ) +
                              "</b></p>"
                              "<p>Jung-Kyu Park (<a href=\"https://github.com/bagjunggyu\">bagjunggyu</a>)</p>"
                              "<p>이정희 (<a href=\"https://hosted.weblate.org/user/MarongHappy/\">MarongHappy</a>)</p>"
                              "<p><b>" +
                              tr ( "Slovak" ) +
                              "</b></p>"
                              "<p>Jose Riha (<a href=\"https://github.com/jose1711\">jose1711</a>)</p>" +
                              "<p><b>" + tr ( "Simplified Chinese" ) +
                              "</b></p>"
                              "<p>Gary Wang (<a href=\"https://github.com/BLumia\">BLumia</a>)</p>" +
                              "<p><b>" + tr ( "Norwegian Bokmål" ) +
                              "</b></p>"
                              "<p>Allan Nordhøy (<a href=\"https://hosted.weblate.org/user/kingu/\">kingu</a>)</p>" );

    // set version number in about dialog
    lblVersion->setText ( GetVersionAndNameStr() );

    // set window title
    setWindowTitle ( tr ( "About %1" ).arg ( APP_NAME ) );

    //### TODO: BEGIN ###//
    // Test if the window also needs to be maximized on Android.
    // Android has not been tested
#    if defined( ANDROID ) || defined( Q_OS_IOS )
    // for mobile version maximize the window
    setWindowState ( Qt::WindowMaximized );
#    endif
    //### TODO: END ###//
}

// Licence dialog --------------------------------------------------------------
CLicenceDlg::CLicenceDlg ( QWidget* parent ) : CBaseDlg ( parent )
{
    /*
        The licence dialog is structured as follows:
        - text box with the licence text on the top
        - check box: I &agree to the above licence terms
        - Accept button (disabled if check box not checked)
        - Decline button
    */
    setWindowIcon ( QIcon ( QString::fromUtf8 ( ":/png/main/res/fronticon.png" ) ) );

    QVBoxLayout* pLayout    = new QVBoxLayout ( this );
    QHBoxLayout* pSubLayout = new QHBoxLayout;
    QLabel*      lblLicence =
        new QLabel ( tr ( "This server requires you accept conditions before you can join. Please read these in the chat window." ), this );
    QCheckBox* chbAgree     = new QCheckBox ( tr ( "I have read the conditions and &agree." ), this );
    butAccept               = new QPushButton ( tr ( "Accept" ), this );
    QPushButton* butDecline = new QPushButton ( tr ( "Decline" ), this );

    pSubLayout->addStretch();
    pSubLayout->addWidget ( chbAgree );
    pSubLayout->addWidget ( butAccept );
    pSubLayout->addWidget ( butDecline );
    pLayout->addWidget ( lblLicence );
    pLayout->addLayout ( pSubLayout );

    // set some properties
    butAccept->setEnabled ( false );
    butAccept->setDefault ( true );

    QObject::connect ( chbAgree, &QCheckBox::stateChanged, this, &CLicenceDlg::OnAgreeStateChanged );

    QObject::connect ( butAccept, &QPushButton::clicked, this, &CLicenceDlg::accept );

    QObject::connect ( butDecline, &QPushButton::clicked, this, &CLicenceDlg::reject );
}

// Help menu -------------------------------------------------------------------
CHelpMenu::CHelpMenu ( const bool bIsClient, QWidget* parent ) : QMenu ( tr ( "&Help" ), parent )
{
    QAction* pAction;

    // standard help menu consists of about and what's this help
    if ( bIsClient )
    {
        addAction ( tr ( "Getting &Started..." ), this, SLOT ( OnHelpClientGetStarted() ) );
        addAction ( tr ( "Software &Manual..." ), this, SLOT ( OnHelpSoftwareMan() ) );
    }
    else
    {
        addAction ( tr ( "Getting &Started..." ), this, SLOT ( OnHelpServerGetStarted() ) );
    }
    addSeparator();
    addAction ( tr ( "What's &This" ), this, SLOT ( OnHelpWhatsThis() ), QKeySequence ( Qt::SHIFT + Qt::Key_F1 ) );
    addSeparator();
    pAction = addAction ( tr ( "&About Jamulus..." ), this, SLOT ( OnHelpAbout() ) );
    pAction->setMenuRole ( QAction::AboutRole ); // required for Mac
    pAction = addAction ( tr ( "About &Qt..." ), this, SLOT ( OnHelpAboutQt() ) );
    pAction->setMenuRole ( QAction::AboutQtRole ); // required for Mac
}

// Language combo box ----------------------------------------------------------
CLanguageComboBox::CLanguageComboBox ( QWidget* parent ) : QComboBox ( parent ), iIdxSelectedLanguage ( INVALID_INDEX )
{
    // This requires a Qt::QueuedConnection on iOS due to https://bugreports.qt.io/browse/QTBUG-64577
    QObject::connect ( this,
                       static_cast<void ( QComboBox::* ) ( int )> ( &QComboBox::activated ),
                       this,
                       &CLanguageComboBox::OnLanguageActivated,
                       Qt::QueuedConnection );
}

void CLanguageComboBox::Init ( QString& strSelLanguage )
{
    // load available translations
    const QMap<QString, QString>   TranslMap = CLocale::GetAvailableTranslations();
    QMapIterator<QString, QString> MapIter ( TranslMap );

    // add translations to the combobox list
    clear();
    int iCnt                  = 0;
    int iIdxOfEnglishLanguage = 0;
    iIdxSelectedLanguage      = INVALID_INDEX;

    while ( MapIter.hasNext() )
    {
        MapIter.next();
        addItem ( QLocale ( MapIter.key() ).nativeLanguageName() + " (" + MapIter.key() + ")", MapIter.key() );

        // store the combo box index of the default english language
        if ( MapIter.key().compare ( "en" ) == 0 )
        {
            iIdxOfEnglishLanguage = iCnt;
        }

        // if the selected language is found, store the combo box index
        if ( MapIter.key().compare ( strSelLanguage ) == 0 )
        {
            iIdxSelectedLanguage = iCnt;
        }

        iCnt++;
    }

    // if the selected language was not found, use the english language
    if ( iIdxSelectedLanguage == INVALID_INDEX )
    {
        strSelLanguage       = "en";
        iIdxSelectedLanguage = iIdxOfEnglishLanguage;
    }

    setCurrentIndex ( iIdxSelectedLanguage );
}

void CLanguageComboBox::OnLanguageActivated ( int iLanguageIdx )
{
    // only update if the language selection is different from the current selected language
    if ( iIdxSelectedLanguage != iLanguageIdx )
    {
        QMessageBox::information ( this, tr ( "Restart Required" ), tr ( "Please restart the application for the language change to take effect." ) );

        emit LanguageChanged ( itemData ( iLanguageIdx ).toString() );
    }
}

QSize CMinimumStackedLayout::sizeHint() const
{
    // always use the size of the currently visible widget:
    if ( currentWidget() )
    {
        return currentWidget()->sizeHint();
    }
    return QStackedLayout::sizeHint();
}
#endif

#endif


/******************************************************************************\
* Other Classes                                                                *
\******************************************************************************/
// Network utility functions ---------------------------------------------------
bool NetworkUtil::ParseNetworkAddressString ( QString strAddress,
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
                                               QString& InetAddr,
#else
                                               QHostAddress& InetAddr,
#endif
                                               bool bEnableIPv6 )
{
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
    if ( strAddress.isEmpty() )
    {
        return false;
    }

    struct addrinfo hints;
    memset ( &hints, 0, sizeof ( hints ) );
    hints.ai_family   = bEnableIPv6 ? AF_UNSPEC : AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    struct addrinfo* res = nullptr;

    if ( getaddrinfo ( strAddress.toStdString().c_str(), nullptr, &hints, &res ) == 0 && res != nullptr )
    {
        for ( struct addrinfo* p = res; p != nullptr; p = p->ai_next )
        {
            if ( p->ai_family == AF_INET )
            {
                char addrBuf[INET_ADDRSTRLEN];
                const struct sockaddr_in* ipv4 = (const struct sockaddr_in*)p->ai_addr;
                if ( inet_ntop ( AF_INET, &ipv4->sin_addr, addrBuf, sizeof ( addrBuf ) ) != nullptr )
                {
                    InetAddr = QString::fromUtf8 ( addrBuf );
                    freeaddrinfo ( res );
                    return true;
                }
            }
        }

        if ( bEnableIPv6 )
        {
            for ( struct addrinfo* p = res; p != nullptr; p = p->ai_next )
            {
                if ( p->ai_family == AF_INET6 )
                {
                    char addrBuf[INET6_ADDRSTRLEN];
                    const struct sockaddr_in6* ipv6 = (const struct sockaddr_in6*)p->ai_addr;
                    if ( inet_ntop ( AF_INET6, &ipv6->sin6_addr, addrBuf, sizeof ( addrBuf ) ) != nullptr )
                    {
                        InetAddr = QString::fromUtf8 ( addrBuf );
                        freeaddrinfo ( res );
                        return true;
                    }
                }
            }
        }
        freeaddrinfo ( res );
    }
    return false;
#else
    if ( strAddress.isEmpty() )
    {
        return false;
    }

    struct addrinfo hints;
    memset ( &hints, 0, sizeof ( hints ) );
    hints.ai_family   = bEnableIPv6 ? AF_UNSPEC : AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    struct addrinfo* res = nullptr;

    if ( getaddrinfo ( strAddress.toStdString().c_str(), nullptr, &hints, &res ) == 0 && res != nullptr )
    {
        for ( struct addrinfo* p = res; p != nullptr; p = p->ai_next )
        {
            if ( p->ai_family == AF_INET )
            {
                char addrBuf[INET_ADDRSTRLEN];
                const struct sockaddr_in* ipv4 = (const struct sockaddr_in*)p->ai_addr;
                if ( inet_ntop ( AF_INET, &ipv4->sin_addr, addrBuf, sizeof ( addrBuf ) ) != nullptr )
                {
                    QHostAddress hostAddr;
                    if ( hostAddr.setAddress ( QString::fromUtf8 ( addrBuf ) ) )
                    {
                        InetAddr = hostAddr;
                        freeaddrinfo ( res );
                        return true;
                    }
                }
            }
        }

        if ( bEnableIPv6 )
        {
            for ( struct addrinfo* p = res; p != nullptr; p = p->ai_next )
            {
                if ( p->ai_family == AF_INET6 )
                {
                    char addrBuf[INET6_ADDRSTRLEN];
                    const struct sockaddr_in6* ipv6 = (const struct sockaddr_in6*)p->ai_addr;
                    if ( inet_ntop ( AF_INET6, &ipv6->sin6_addr, addrBuf, sizeof ( addrBuf ) ) != nullptr )
                    {
                        QHostAddress hostAddr;
                        if ( hostAddr.setAddress ( QString::fromUtf8 ( addrBuf ) ) )
                        {
                            InetAddr = hostAddr;
                            freeaddrinfo ( res );
                            return true;
                        }
                    }
                }
            }
        }
        freeaddrinfo ( res );
    }
    return false;
#endif
}


#ifndef CLIENT_NO_SRV_CONNECT
bool NetworkUtil::ParseNetworkAddressSrv ( QString strAddress, CHostAddress& HostAddress, bool bEnableIPv6 )
{
    HostAddress = CHostAddress();
    if ( strAddress.isEmpty() )
    {
        return false;
    }

    const QChar firstChar = strAddress.at ( 0 );
    if ( firstChar == '[' || firstChar == ':' || firstChar.isDigit() || firstChar == '.' )
    {
        return false;
    }

    if ( strAddress.contains ( ":" ) )
    {
        return false;
    }

#if defined( JAMULUS_USE_JUCE_NET )
    JUCE_Resolver resolver;
    const auto queryName = QStringLiteral( "_jamulus._udp." ) + strAddress;
    const auto endpoints = resolver.resolveSrv ( queryName.toStdString() );
    if ( endpoints.empty() )
    {
        return false;
    }

    const auto& endpoint = endpoints.front();
    if ( endpoint.address.empty() )
    {
        HostAddress = CHostAddress ( std::string ( "." ), 0 );
        return true;
    }

    QString inetAddr = QString::fromUtf8 ( endpoint.address.c_str() );
    QString parsedAddr;
    if ( ParseNetworkAddressString ( inetAddr, parsedAddr, bEnableIPv6 ) )
    {
        HostAddress = CHostAddress ( parsedAddr.toStdString(), static_cast<uint16_t> ( endpoint.port ) );
        return true;
    }
    return false;
#elif defined( HEADLESS )
    Q_UNUSED ( bEnableIPv6 );
    return false;
#else
    QDnsLookup* dns = new QDnsLookup();
    dns->setType ( QDnsLookup::SRV );
    dns->setName ( QString ( "_jamulus._udp.%1" ).arg ( strAddress ) );
    dns->lookup();
    QTime dieTime = QTime::currentTime().addMSecs ( DNS_SRV_RESOLVE_TIMEOUT_MS );
    while ( QTime::currentTime() < dieTime && !dns->isFinished() )
    {
        QCoreApplication::processEvents ( QEventLoop::ExcludeUserInputEvents, 100 );
    }
    QList<QDnsServiceRecord> records = dns->serviceRecords();
    dns->deleteLater();
    if ( records.length() != 1 )
    {
        return false;
    }
    QDnsServiceRecord record = records.first();
    if ( record.target() == "." || record.target() == "" )
    {
        HostAddress = CHostAddress ( QHostAddress ( "." ), 0 );
        return true;
    }

    QHostAddress InetAddr;
    if ( ParseNetworkAddressString ( record.target(), InetAddr, bEnableIPv6 ) )
    {
        HostAddress = CHostAddress ( InetAddr, record.port() );
        return true;
    }
    return false;
#endif
}

bool NetworkUtil::ParseNetworkAddressWithSrvDiscovery ( QString strAddress, CHostAddress& HostAddress, bool bEnableIPv6 )
{
    // Try SRV-based discovery first:
    if ( ParseNetworkAddressSrv ( strAddress, HostAddress, bEnableIPv6 ) )
    {
        return true;
    }
    // Try regular connect via plain IP or host name lookup (A/AAAA):
    return ParseNetworkAddressBare ( strAddress, HostAddress, bEnableIPv6 );
}
#endif

bool NetworkUtil::ParseNetworkAddressBare ( QString strAddress, CHostAddress& HostAddress, bool bEnableIPv6 )
{
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
    QString InetAddr;
#else
    QHostAddress InetAddr;
#endif
    unsigned int iNetPort = DEFAULT_PORT_NUMBER;

    // qInfo() << qUtf8Printable ( QString ( "Parsing network address %1" ).arg ( strAddress ) );

    HostAddress = CHostAddress();

    // Allow the following address formats:
    // [addr4or6]
    // [addr4or6]:port
    // addr4
    // addr4:port
    // hostname
    // hostname:port
    // (where addr4or6 is a literal IPv4 or IPv6 address, and addr4 is a literal IPv4 address

    bool    bLiteralAddr = false;
    QString strPort;

    if ( strAddress.startsWith ( "[" ) )
    {
        const int closingIdx = strAddress.indexOf ( "]" );
        if ( closingIdx < 0 )
        {
            return false;
        }

        if ( closingIdx + 1 < strAddress.length() )
        {
            if ( strAddress.at ( closingIdx + 1 ) != ':' )
            {
                return false;
            }

            strPort = strAddress.mid ( closingIdx + 2 );
            if ( strPort.isEmpty() )
            {
                return false;
            }
        }

        strAddress   = strAddress.mid ( 1, closingIdx - 1 );
        bLiteralAddr = true;
    }
    else
    {
        const int firstColon = strAddress.indexOf ( ":" );
        if ( firstColon >= 0 )
        {
            if ( strAddress.indexOf ( ":", firstColon + 1 ) != -1 )
            {
                return false;
            }

            strPort = strAddress.mid ( firstColon + 1 );
            if ( strPort.isEmpty() )
            {
                return false;
            }

            strAddress = strAddress.left ( firstColon );
        }
    }

    if ( !strPort.isEmpty() )
    {
        // a port number was given: extract port number
        bool ok  = false;
        iNetPort = strPort.toInt ( &ok );

        if ( !ok )
        {
            return false;
        }

        if ( iNetPort >= 65536 )
        {
            // invalid port number
            // qInfo() << qUtf8Printable ( QString ( "Invalid port number specified" ) );
            return false;
        }
    }

#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
    bool isIPv6Literal = false;
    std::string addrStr = strAddress.toStdString();
    const char* addrBytes = addrStr.c_str();
    struct in_addr addr4;
    struct in6_addr addr6;
    if ( inet_pton ( AF_INET, addrBytes, &addr4 ) == 1 )
    {
        InetAddr = strAddress;
    }
    else if ( bEnableIPv6 && inet_pton ( AF_INET6, addrBytes, &addr6 ) == 1 )
    {
        InetAddr = strAddress;
        isIPv6Literal = true;
    }

    if ( !InetAddr.isEmpty() )
    {
        if ( !bEnableIPv6 && isIPv6Literal )
        {
            return false;
        }
    }
    else
    {
        if ( bLiteralAddr )
        {
            return false;
        }

        if ( !ParseNetworkAddressString ( strAddress, InetAddr, bEnableIPv6 ) )
        {
            return false;
        }
    }

    HostAddress = CHostAddress (
#if defined( JAMULUS_USE_JUCE_NET )
        InetAddr.toStdString(),
#else
        InetAddr,
#endif
        static_cast<uint16_t> ( iNetPort ) );
    return true;
#else
    if ( InetAddr.setAddress ( strAddress ) )
    {
        if ( !bEnableIPv6 && InetAddr.protocol() == QAbstractSocket::IPv6Protocol )
        {
            // do not allow IPv6 addresses if not enabled
            // qInfo() << qUtf8Printable ( QString ( "IPv6 addresses disabled" ) );
            return false;
        }
    }
    else
    {
        // it was no valid IP address. If literal required, return as invalid
        if ( bLiteralAddr )
        {
            // qInfo() << qUtf8Printable ( QString ( "Invalid literal IP address" ) );
            return false; // invalid address
        }

        if ( !ParseNetworkAddressString ( strAddress, InetAddr, bEnableIPv6 ) )
        {
            // no valid address found
            // qInfo() << qUtf8Printable ( QString ( "No IP address found for hostname" ) );
            return false;
        }
    }

    HostAddress = CHostAddress (
#if defined( JAMULUS_USE_JUCE_NET )
        InetAddr.toStdString(),
#else
        InetAddr,
#endif
        static_cast<uint16_t> ( iNetPort ) );
    return true;
#endif
}

bool NetworkUtil::ParseNetworkAddress ( QString strAddress, CHostAddress& HostAddress, bool bEnableIPv6 )
{
#ifndef CLIENT_NO_SRV_CONNECT
    if ( ParseNetworkAddressSrv ( strAddress, HostAddress, bEnableIPv6 ) )
    {
        return true;
    }
#endif
    return ParseNetworkAddressBare ( strAddress, HostAddress, bEnableIPv6 );
}

#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
bool NetworkUtil::ParseNetworkAddressStd ( const std::string& strAddress, CHostAddress& HostAddress, bool bEnableIPv6 )
{
    return ParseNetworkAddress ( QString::fromStdString ( strAddress ), HostAddress, bEnableIPv6 );
}
#endif

CHostAddress NetworkUtil::GetLocalAddress()
{
    auto addrs    = juce::IPAddress::getAllAddresses();
    const auto localV4 = juce::IPAddress::local ( false );
    for ( auto& ip : addrs )
    {
        if ( !ip.isIPv6 && ip != localV4 )
        {
#if defined( JAMULUS_USE_JUCE_NET )
            auto ipString = ip.toString();
            return CHostAddress ( std::string ( ipString.toRawUTF8() ), 0 );
#else
            auto ipString = QString::fromUtf8 ( ip.toString().toRawUTF8() );
            return CHostAddress ( QHostAddress ( ipString ), 0 );
#endif
        }
    }

#if defined( JAMULUS_USE_JUCE_NET )
    return CHostAddress ( std::string ( "127.0.0.1" ), 0 );
#else
    return CHostAddress ( QHostAddress::LocalHost, 0 );
#endif
}

CHostAddress NetworkUtil::GetLocalAddress6()
{
    auto addrs    = juce::IPAddress::getAllAddresses();
    const auto localV6 = juce::IPAddress::local ( true );
    for ( auto& ip : addrs )
    {
        if ( ip.isIPv6 && ip != localV6 )
        {
#if defined( JAMULUS_USE_JUCE_NET )
            auto ipString = ip.toString();
            return CHostAddress ( std::string ( ipString.toRawUTF8() ), 0 );
#else
            auto ipString = QString::fromUtf8 ( ip.toString().toRawUTF8() );
            return CHostAddress ( QHostAddress ( ipString ), 0 );
#endif
        }
    }

#if defined( JAMULUS_USE_JUCE_NET )
    return CHostAddress ( std::string ( "::1" ), 0 );
#else
    return CHostAddress ( QHostAddress::LocalHostIPv6, 0 );
#endif
}

QString NetworkUtil::GetDirectoryAddress ( const EDirectoryType eDirectoryType, const QString& strDirectoryAddress )
{
    switch ( eDirectoryType )
    {
    case AT_CUSTOM:
        return strDirectoryAddress;
    case AT_ANY_GENRE2:
        return CENTSERV_ANY_GENRE2;
    case AT_ANY_GENRE3:
        return CENTSERV_ANY_GENRE3;
    case AT_GENRE_ROCK:
        return CENTSERV_GENRE_ROCK;
    case AT_GENRE_JAZZ:
        return CENTSERV_GENRE_JAZZ;
    case AT_GENRE_CLASSICAL_FOLK:
        return CENTSERV_GENRE_CLASSICAL_FOLK;
    case AT_GENRE_CHORAL:
        return CENTSERV_GENRE_CHORAL;
    default:
        return DEFAULT_SERVER_ADDRESS; // AT_DEFAULT
    }
}

QString NetworkUtil::FixAddress ( const QString& strAddress )
{
    // remove all spaces from the address string
    return strAddress.simplified().replace ( " ", "" );
}

// Return whether the given HostAdress is within a private IP range
// as per RFC 1918 & RFC 5735.
bool NetworkUtil::IsLiteralIPAddress ( const QString& strAddress, bool bEnableIPv6 )
{
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
    if ( strAddress.isEmpty() )
    {
        return false;
    }

    std::string addrStr = strAddress.toStdString();
    const char* addrBytes = addrStr.c_str();
    struct in_addr addr4;
    if ( inet_pton ( AF_INET, addrBytes, &addr4 ) == 1 )
    {
        return true;
    }
    if ( bEnableIPv6 )
    {
        struct in6_addr addr6;
        if ( inet_pton ( AF_INET6, addrBytes, &addr6 ) == 1 )
        {
            return true;
        }
    }
    return false;
#else
    QHostAddress InetAddr;
    if ( !InetAddr.setAddress ( strAddress ) )
    {
        return false;
    }
    if ( !bEnableIPv6 && InetAddr.protocol() == QAbstractSocket::IPv6Protocol )
    {
        return false;
    }
    return true;
#endif
}

#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
bool NetworkUtil::IsPrivateNetworkIP ( const QString& address )
{
    if ( address.isEmpty() )
    {
        return false;
    }

    struct in_addr addr4;
    if ( inet_pton ( AF_INET, address.toUtf8().constData(), &addr4 ) != 1 )
    {
        return false;
    }

    const quint32 addr = ntohl ( addr4.s_addr );

    auto inSubnet = [addr] ( quint32 base, int prefixLen ) {
        if ( prefixLen <= 0 )
        {
            return false;
        }
        const quint32 mask = prefixLen == 32 ? 0xFFFFFFFFu : ( 0xFFFFFFFFu << ( 32 - prefixLen ) );
        return ( addr & mask ) == ( base & mask );
    };

    if ( inSubnet ( 0x0A000000u, 8 ) )
    {
        return true;
    }

    if ( inSubnet ( 0x7F000000u, 8 ) )
    {
        return true;
    }

    if ( inSubnet ( 0xAC100000u, 12 ) )
    {
        return true;
    }

    if ( inSubnet ( 0xC0A80000u, 16 ) )
    {
        return true;
    }

    return false;
}

bool NetworkUtil::IsPrivateNetworkIPStd ( const std::string& address )
{
    if ( address.empty() )
    {
        return false;
    }

    struct in_addr addr4;
    if ( inet_pton ( AF_INET, address.c_str(), &addr4 ) != 1 )
    {
        return false;
    }

    const quint32 addr = ntohl ( addr4.s_addr );

    auto inSubnet = [addr] ( quint32 base, int prefixLen ) {
        if ( prefixLen <= 0 )
        {
            return false;
        }
        const quint32 mask = prefixLen == 32 ? 0xFFFFFFFFu : ( 0xFFFFFFFFu << ( 32 - prefixLen ) );
        return ( addr & mask ) == ( base & mask );
    };

    if ( inSubnet ( 0x0A000000u, 8 ) )
    {
        return true;
    }

    if ( inSubnet ( 0x7F000000u, 8 ) )
    {
        return true;
    }

    if ( inSubnet ( 0xAC100000u, 12 ) )
    {
        return true;
    }

    if ( inSubnet ( 0xC0A80000u, 16 ) )
    {
        return true;
    }

    return false;
}
#else
bool NetworkUtil::IsPrivateNetworkIP ( const QHostAddress& qhAddr )
{
    // https://www.rfc-editor.org/rfc/rfc1918
    // https://www.rfc-editor.org/rfc/rfc5735
    static QList<QPair<QHostAddress, int>> addresses = {
        QPair<QHostAddress, int> ( QHostAddress ( "10.0.0.0" ), 8 ),
        QPair<QHostAddress, int> ( QHostAddress ( "127.0.0.0" ), 8 ),
        QPair<QHostAddress, int> ( QHostAddress ( "172.16.0.0" ), 12 ),
        QPair<QHostAddress, int> ( QHostAddress ( "192.168.0.0" ), 16 ),
    };

    foreach ( auto item, addresses )
    {
        if ( qhAddr.isInSubnet ( item ) )
        {
            return true;
        }
    }
    return false;
}
#endif

#if defined( JAMULUS_USE_JUCE_NET )
int CHostAddress::Compare ( const CHostAddress& other ) const
{
    if ( iPort != other.iPort )
    {
        return (int) iPort - (int) other.iPort;
    }

    if ( address == other.address )
    {
        return 0;
    }

    return address < other.address ? -1 : 1;
}

std::string CHostAddress::toStringStd ( const EStringMode eStringMode ) const
{
    std::string strReturn = address;

    if ( ( eStringMode == SM_IP_NO_LAST_BYTE ) || ( eStringMode == SM_IP_NO_LAST_BYTE_PORT ) )
    {
        const auto dotPos   = strReturn.find_last_of ( '.' );
        const auto colonPos = strReturn.find_last_of ( ':' );

        if ( dotPos != std::string::npos && ( colonPos == std::string::npos || dotPos > colonPos ) )
        {
            strReturn = strReturn.substr ( 0, dotPos ) + ".x";
        }
        else if ( colonPos != std::string::npos )
        {
            strReturn = strReturn.substr ( 0, colonPos ) + ":x";
        }
    }

    if ( ( eStringMode == SM_IP_PORT ) || ( eStringMode == SM_IP_NO_LAST_BYTE_PORT ) )
    {
        if ( strReturn.find ( '.' ) != std::string::npos )
        {
            strReturn += ":" + std::to_string ( iPort );
        }
        else
        {
            strReturn = "[" + strReturn + "]:" + std::to_string ( iPort );
        }
    }

    return strReturn;
}

QString CHostAddress::toString ( const EStringMode eStringMode ) const
{
    return QString::fromStdString ( toStringStd ( eStringMode ) );
}
#elif defined( HEADLESS )
int CHostAddress::Compare ( const CHostAddress& other ) const
{
    if ( iPort != other.iPort )
    {
        return (int) iPort - (int) other.iPort;
    }

    if ( address == other.address )
    {
        return 0;
    }

    return address < other.address ? -1 : 1;
}

QString CHostAddress::toString ( const EStringMode eStringMode ) const
{
    QString strReturn = QString::fromStdString ( address );

    if ( ( eStringMode == SM_IP_NO_LAST_BYTE ) || ( eStringMode == SM_IP_NO_LAST_BYTE_PORT ) )
    {
        if ( strReturn.contains ( "." ) )
        {
            strReturn = strReturn.section ( ".", 0, -2 ) + ".x";
        }
        else if ( strReturn.contains ( ":" ) )
        {
            strReturn = strReturn.section ( ":", 0, -2 ) + ":x";
        }
    }

    if ( ( eStringMode == SM_IP_PORT ) || ( eStringMode == SM_IP_NO_LAST_BYTE_PORT ) )
    {
        if ( strReturn.contains ( "." ) )
        {
            strReturn += ":" + QString().setNum ( iPort );
        }
        else
        {
            strReturn = "[" + strReturn + "]:" + QString().setNum ( iPort );
        }
    }

    return strReturn;
}
#else
int CHostAddress::Compare ( const CHostAddress& other ) const
{
    if ( iPort != other.iPort )
    {
        return (int) iPort - (int) other.iPort;
    }

    QAbstractSocket::NetworkLayerProtocol thisProto  = InetAddr.protocol();
    QAbstractSocket::NetworkLayerProtocol otherProto = other.InetAddr.protocol();

    if ( thisProto != otherProto )
    {
        return (int) thisProto - (int) otherProto;
    }

    if ( thisProto == QAbstractSocket::IPv6Protocol )
    {
        Q_IPV6ADDR thisAddr  = InetAddr.toIPv6Address();
        Q_IPV6ADDR otherAddr = other.InetAddr.toIPv6Address();

        return memcmp ( &thisAddr, &otherAddr, sizeof ( Q_IPV6ADDR ) );
    }

    quint32 thisAddr  = InetAddr.toIPv4Address();
    quint32 otherAddr = other.InetAddr.toIPv4Address();

    return thisAddr < otherAddr ? -1 : thisAddr > otherAddr ? 1 : 0;
}

QString CHostAddress::toString ( const EStringMode eStringMode ) const
{
    QString strReturn = InetAddr.toString();

    if ( ( ( eStringMode == SM_IP_NO_LAST_BYTE ) || ( eStringMode == SM_IP_NO_LAST_BYTE_PORT ) ) &&
         ( InetAddr != QHostAddress ( QHostAddress::LocalHost ) ) && ( InetAddr != QHostAddress ( QHostAddress::LocalHostIPv6 ) ) )
    {
        if ( strReturn.contains ( "." ) )
        {
            strReturn = strReturn.section ( ".", 0, -2 ) + ".x";
        }
        else
        {
            strReturn = strReturn.section ( ":", 0, -2 ) + ":x";
        }
    }

    if ( ( eStringMode == SM_IP_PORT ) || ( eStringMode == SM_IP_NO_LAST_BYTE_PORT ) )
    {
        if ( strReturn.contains ( "." ) )
        {
            strReturn += ":" + QString().setNum ( iPort );
        }
        else
        {
            strReturn = "[" + strReturn + "]:" + QString().setNum ( iPort );
        }
    }

    return strReturn;
}
#endif

// Instrument picture data base ------------------------------------------------
CVector<CInstPictures::CInstPictProps>& CInstPictures::GetTable ( const bool bReGenerateTable )
{
    // make sure we generate the table only once
    static bool TableIsInitialized = false;

    static CVector<CInstPictProps> vecDataBase;

    if ( !TableIsInitialized || bReGenerateTable )
    {
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
        auto trCS = [] ( const char* s ) { return QString::fromUtf8 ( s ); };
#else
        auto trCS = [] ( const char* s ) { return QCoreApplication::translate ( "CClientSettingsDlg", s ); };
#endif
        // instrument picture data base initialization
        // NOTE: Do not change the order of any instrument in the future!
        // NOTE: The very first entry is the "not used" element per definition.
        vecDataBase.Init ( 0 ); // first clear all existing data since we create the list be adding entries
        vecDataBase.Add ( CInstPictProps ( trCS ( "None" ),
                                           ":/png/instr/res/instruments/none.png",
                                           IC_OTHER_INSTRUMENT ) ); // special first element
        vecDataBase.Add ( CInstPictProps ( trCS ( "Drum Set" ),
                                           ":/png/instr/res/instruments/drumset.png",
                                           IC_PERCUSSION_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Djembe" ),
                                           ":/png/instr/res/instruments/djembe.png",
                                           IC_PERCUSSION_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Electric Guitar" ),
                                           ":/png/instr/res/instruments/eguitar.png",
                                           IC_PLUCKING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Acoustic Guitar" ),
                                           ":/png/instr/res/instruments/aguitar.png",
                                           IC_PLUCKING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Bass Guitar" ),
                                           ":/png/instr/res/instruments/bassguitar.png",
                                           IC_PLUCKING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Keyboard" ),
                                           ":/png/instr/res/instruments/keyboard.png",
                                           IC_KEYBOARD_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Synthesizer" ),
                                           ":/png/instr/res/instruments/synthesizer.png",
                                           IC_KEYBOARD_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Grand Piano" ),
                                           ":/png/instr/res/instruments/grandpiano.png",
                                           IC_KEYBOARD_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Accordion" ),
                                           ":/png/instr/res/instruments/accordeon.png",
                                           IC_KEYBOARD_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Vocal" ),
                                           ":/png/instr/res/instruments/vocal.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Microphone" ),
                                           ":/png/instr/res/instruments/microphone.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Harmonica" ),
                                           ":/png/instr/res/instruments/harmonica.png",
                                           IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Trumpet" ),
                                           ":/png/instr/res/instruments/trumpet.png",
                                           IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Trombone" ),
                                           ":/png/instr/res/instruments/trombone.png",
                                           IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "French Horn" ),
                                           ":/png/instr/res/instruments/frenchhorn.png",
                                           IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Tuba" ),
                                           ":/png/instr/res/instruments/tuba.png",
                                           IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Saxophone" ),
                                           ":/png/instr/res/instruments/saxophone.png",
                                           IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Clarinet" ),
                                           ":/png/instr/res/instruments/clarinet.png",
                                           IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Flute" ),
                                           ":/png/instr/res/instruments/flute.png",
                                           IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Violin" ),
                                           ":/png/instr/res/instruments/violin.png",
                                           IC_STRING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Cello" ),
                                           ":/png/instr/res/instruments/cello.png",
                                           IC_STRING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Double Bass" ),
                                           ":/png/instr/res/instruments/doublebass.png",
                                           IC_STRING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Recorder" ),
                                           ":/png/instr/res/instruments/recorder.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Streamer" ),
                                           ":/png/instr/res/instruments/streamer.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Listener" ),
                                           ":/png/instr/res/instruments/listener.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Guitar+Vocal" ),
                                           ":/png/instr/res/instruments/guitarvocal.png",
                                           IC_MULTIPLE_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Keyboard+Vocal" ),
                                           ":/png/instr/res/instruments/keyboardvocal.png",
                                           IC_MULTIPLE_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Bodhran" ),
                                           ":/png/instr/res/instruments/bodhran.png",
                                           IC_PERCUSSION_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Bassoon" ),
                                           ":/png/instr/res/instruments/bassoon.png",
                                           IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Oboe" ),
                                           ":/png/instr/res/instruments/oboe.png",
                                           IC_WIND_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Harp" ),
                                           ":/png/instr/res/instruments/harp.png",
                                           IC_STRING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Viola" ),
                                           ":/png/instr/res/instruments/viola.png",
                                           IC_STRING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Congas" ),
                                           ":/png/instr/res/instruments/congas.png",
                                           IC_PERCUSSION_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Bongo" ),
                                           ":/png/instr/res/instruments/bongo.png",
                                           IC_PERCUSSION_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Vocal Bass" ),
                                           ":/png/instr/res/instruments/vocalbass.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Vocal Tenor" ),
                                           ":/png/instr/res/instruments/vocaltenor.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Vocal Alto" ),
                                           ":/png/instr/res/instruments/vocalalto.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Vocal Soprano" ),
                                           ":/png/instr/res/instruments/vocalsoprano.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Banjo" ),
                                           ":/png/instr/res/instruments/banjo.png",
                                           IC_PLUCKING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Mandolin" ),
                                           ":/png/instr/res/instruments/mandolin.png",
                                           IC_PLUCKING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Ukulele" ),
                                           ":/png/instr/res/instruments/ukulele.png",
                                           IC_PLUCKING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Bass Ukulele" ),
                                           ":/png/instr/res/instruments/bassukulele.png",
                                           IC_PLUCKING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Vocal Baritone" ),
                                           ":/png/instr/res/instruments/vocalbaritone.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Vocal Lead" ),
                                           ":/png/instr/res/instruments/vocallead.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Mountain Dulcimer" ),
                                           ":/png/instr/res/instruments/mountaindulcimer.png",
                                           IC_STRING_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Scratching" ),
                                           ":/png/instr/res/instruments/scratching.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Rapping" ),
                                           ":/png/instr/res/instruments/rapping.png",
                                           IC_OTHER_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Vibraphone" ),
                                           ":/png/instr/res/instruments/vibraphone.png",
                                           IC_PERCUSSION_INSTRUMENT ) );
        vecDataBase.Add ( CInstPictProps ( trCS ( "Conductor" ),
                                           ":/png/instr/res/instruments/conductor.png",
                                           IC_OTHER_INSTRUMENT ) );

        // now the table is initialized
        TableIsInitialized = true;
    }

    return vecDataBase;
}

bool CInstPictures::IsInstIndexInRange ( const int iIdx )
{
    // check if index is in valid range
    return ( iIdx >= 0 ) && ( iIdx < GetTable().Size() );
}

QString CInstPictures::GetResourceReference ( const int iInstrument )
{
    // range check
    if ( IsInstIndexInRange ( iInstrument ) )
    {
        // return the string of the resource reference for accessing the picture
        return GetTable()[iInstrument].strResourceReference;
    }
    else
    {
        return "";
    }
}

QString CInstPictures::GetName ( const int iInstrument )
{
    // range check
    if ( IsInstIndexInRange ( iInstrument ) )
    {
        // return the name of the instrument
        return GetTable()[iInstrument].strName;
    }
    else
    {
        return "";
    }
}

CInstPictures::EInstCategory CInstPictures::GetCategory ( const int iInstrument )
{
    // range check
    if ( IsInstIndexInRange ( iInstrument ) )
    {
        // return the name of the instrument
        return GetTable()[iInstrument].eInstCategory;
    }
    else
    {
        return IC_OTHER_INSTRUMENT;
    }
}

// Locale management class -----------------------------------------------------
QCOUNTRY_T CLocale::WireFormatCountryCodeToQtCountry ( unsigned short iCountryCode )
{
#if !defined( JAMULUS_USE_JUCE_NET )
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
    // The Jamulus protocol wire format gives us Qt5 country IDs.
    // Qt6 changed those IDs, so we have to convert back:
    return (QCOUNTRY_T) wireFormatToQt6Table[iCountryCode];
#else
    return (QCOUNTRY_T) iCountryCode;
#endif
#else
    // In JUCE net builds, QCOUNTRY_T is already the wire-format value.
    return (QCOUNTRY_T) iCountryCode;
#endif
}

unsigned short CLocale::QtCountryToWireFormatCountryCode ( const QCOUNTRY_T eCountry )
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
    // The Jamulus protocol wire format expects Qt5 country IDs.
    // Qt6 changed those IDs, so we have to convert back:
    return qt6CountryToWireFormat[(unsigned short) eCountry];
#else
    return (unsigned short) eCountry;
#endif
}

bool CLocale::IsCountryCodeSupported ( unsigned short iCountryCode )
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 0, 0 )
    // On newer Qt versions there might be codes which do not have a Qt5 equivalent.
    // We have no way to support those sanely right now.
    // Before we can check that via an array lookup, we have to ensure that
    // we are within the boundaries of that array:
    if ( iCountryCode >= qt6CountryToWireFormatLen )
    {
        return false;
    }
    return qt6CountryToWireFormat[iCountryCode] != -1;
#else
    // All Qt5 codes are supported.
    return true;
#endif
}

QCOUNTRY_T CLocale::GetCountryCodeByTwoLetterCode ( QString sTwoLetterCode )
{
#if !defined( JAMULUS_USE_JUCE_NET )
#if QT_VERSION >= QT_VERSION_CHECK( 6, 2, 0 )
    return (QCOUNTRY_T) QLocale::codeToTerritory ( sTwoLetterCode );
#else
    QList<QLocale> vLocaleList = QLocale::matchingLocales ( QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry );
    QStringList    vstrLocParts;

    // Qt < 6.2 does not support lookups from two-letter iso codes to
    // QLocale::Country. Therefore, we have to loop over all supported
    // locales and perform the matching ourselves.
    // In the future, QLocale::codeToTerritory can be used.
    foreach ( const QLocale qLocale, vLocaleList )
    {
        QStringList vstrLocParts = qLocale.name().split ( "_" );

        if ( vstrLocParts.size() >= 2 && vstrLocParts.at ( 1 ).toLower() == sTwoLetterCode.toLower() )
        {
            return qLocale.country();
        }
    }
    return (QCOUNTRY_T) QLocale::AnyCountry;
#endif
#else
    // In JUCE net builds we don't depend on QLocale; use 0 ("no country").
    Q_UNUSED ( sTwoLetterCode );
    return (QCOUNTRY_T) 0;
#endif
}

#if !defined( JAMULUS_USE_JUCE_NET )
QString CLocale::GetCountryFlagIconsResourceReference ( const QCOUNTRY_T eCountry /* Qt-native value */ )
{
    QString strReturn = "";

    // special flag for none
    if ( eCountry == QLocale::AnyCountry )
    {
        return ":/png/flags/res/flags/flagnone.png";
    }

    // There is no direct query of the country code in Qt, therefore we use a
    // workaround: Get the matching locales properties and split the name of
    // that since the second part is the country code
    QList<QLocale> vCurLocaleList = QLocale::matchingLocales ( QLocale::AnyLanguage, QLocale::AnyScript, (QLocale::Country) eCountry );

    // check if the matching locales query was successful
    if ( vCurLocaleList.size() < 1 )
    {
        return "";
    }

    QStringList vstrLocParts = vCurLocaleList.at ( 0 ).name().split ( "_" );

    // the second split contains the name we need
    if ( vstrLocParts.size() <= 1 )
    {
        return "";
    }

    strReturn = ":/png/flags/res/flags/" + vstrLocParts.at ( 1 ).toLower() + ".png";

    // check if file actually exists, if not then invalidate reference
    if ( !QFile::exists ( strReturn ) )
    {
        return "";
    }

    return strReturn;
}
#else
QString CLocale::GetCountryFlagIconsResourceReference ( const QCOUNTRY_T )
{
    return "";
}
#endif

QMap<QString, QString> CLocale::GetAvailableTranslations()
{
#if defined( HEADLESS ) || defined( JAMULUS_NO_QT_TRANSLATIONS ) || defined( JAMULUS_USE_JUCE_NET )
    QMap<QString, QString> TranslMap;
    TranslMap["en"] = "";
    return TranslMap;
#else
    QMap<QString, QString> TranslMap;

    QDirIterator DirIter ( ":/i18n" );

    TranslMap["en"] = "";

    while ( DirIter.hasNext() )
    {
        const QString strCurFileName = DirIter.next();

        const int     lang   = strCurFileName.indexOf ( "_" ) + 1;
        const QString strLoc = strCurFileName.mid ( lang, strCurFileName.indexOf ( "." ) - lang );

        TranslMap[strLoc] = strCurFileName;
    }

    return TranslMap;
#endif
}

QPair<QString, QString> CLocale::FindSysLangTransFileName ( const QMap<QString, QString>& TranslMap )
{
#if defined( HEADLESS ) || defined( JAMULUS_NO_QT_TRANSLATIONS ) || defined( JAMULUS_USE_JUCE_NET )
    return QPair<QString, QString> ( "en", "" );
#else
    QPair<QString, QString> PairSysLang ( "", "" );
    QStringList             slUiLang = QLocale().uiLanguages();

    if ( !slUiLang.isEmpty() )
    {
        QString strUiLang = QLocale().uiLanguages().at ( 0 );
        strUiLang.replace ( "-", "_" );

        if ( TranslMap.constFind ( strUiLang ) != TranslMap.constEnd() )
        {
            PairSysLang.first  = strUiLang;
            PairSysLang.second = TranslMap[PairSysLang.first];
        }
        else
        {
            if ( strUiLang.length() >= 2 )
            {
                PairSysLang.first  = strUiLang.left ( 2 );
                PairSysLang.second = TranslMap[PairSysLang.first];
            }
        }
    }

    return PairSysLang;
#endif
}

void CLocale::LoadTranslation ( const QString strLanguage, QCoreApplication* pApp )
{
#if defined( HEADLESS ) || defined( JAMULUS_NO_QT_TRANSLATIONS ) || defined( JAMULUS_USE_JUCE_NET )
    (void) strLanguage;
    (void) pApp;
    return;
#else
    static QTranslator myappTranslator;
    static QTranslator myqtTranslator;

    QMap<QString, QString> TranslMap              = CLocale::GetAvailableTranslations();
    const QString          strTranslationFileName = TranslMap[strLanguage];

    if ( myappTranslator.load ( strTranslationFileName ) )
    {
        pApp->installTranslator ( &myappTranslator );
    }

    if ( myqtTranslator.load ( QLocale ( strLanguage ), "qt", "_", QLibraryInfo::location ( QLibraryInfo::TranslationsPath ) ) )
    {
        pApp->installTranslator ( &myqtTranslator );
    }
#endif
}

/******************************************************************************\
* Global Functions Implementation                                              *
\******************************************************************************/
QString DirectoryTypeToString ( EDirectoryType eAddrType )
{
    switch ( eAddrType )
    {
    case AT_NONE:
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
        return QStringLiteral ( "None" );
#else
        return QCoreApplication::translate ( "CServerDlg", "None" );
#endif

    case AT_ANY_GENRE2:
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
        return QStringLiteral ( "Any Genre 2" );
#else
        return QCoreApplication::translate ( "CClientSettingsDlg", "Any Genre 2" );
#endif

    case AT_ANY_GENRE3:
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
        return QStringLiteral ( "Any Genre 3" );
#else
        return QCoreApplication::translate ( "CClientSettingsDlg", "Any Genre 3" );
#endif

    case AT_GENRE_ROCK:
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
        return QStringLiteral ( "Genre Rock" );
#else
        return QCoreApplication::translate ( "CClientSettingsDlg", "Genre Rock" );
#endif

    case AT_GENRE_JAZZ:
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
        return QStringLiteral ( "Genre Jazz" );
#else
        return QCoreApplication::translate ( "CClientSettingsDlg", "Genre Jazz" );
#endif

    case AT_GENRE_CLASSICAL_FOLK:
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
        return QStringLiteral ( "Genre Classical/Folk" );
#else
        return QCoreApplication::translate ( "CClientSettingsDlg", "Genre Classical/Folk" );
#endif

    case AT_GENRE_CHORAL:
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
        return QStringLiteral ( "Genre Choral/Barbershop" );
#else
        return QCoreApplication::translate ( "CClientSettingsDlg", "Genre Choral/Barbershop" );
#endif

    case AT_CUSTOM:
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
        return QStringLiteral ( "Custom" );
#else
        return QCoreApplication::translate ( "CClientSettingsDlg", "Custom" );
#endif

    default: // AT_DEFAULT
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
        return QStringLiteral ( "Any Genre 1" );
#else
        return QCoreApplication::translate ( "CClientSettingsDlg", "Any Genre 1" );
#endif
    }
}

QString svrRegStatusToString ( ESvrRegStatus eSvrRegStatus )
{
    switch ( eSvrRegStatus )
    {
    case SRS_NOT_REGISTERED:
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
        return QStringLiteral ( "Not registered" );
#else
        return QCoreApplication::translate ( "CServerDlg", "Not registered" );
#endif

    case SRS_BAD_ADDRESS:
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
        return QStringLiteral ( "Bad address" );
#else
        return QCoreApplication::translate ( "CServerDlg", "Bad address" );
#endif

    case SRS_REQUESTED:
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
        return QStringLiteral ( "Registration requested" );
#else
        return QCoreApplication::translate ( "CServerDlg", "Registration requested" );
#endif

    case SRS_TIME_OUT:
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
        return QStringLiteral ( "Registration failed" );
#else
        return QCoreApplication::translate ( "CServerDlg", "Registration failed" );
#endif

    case SRS_UNKNOWN_RESP:
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
        return QStringLiteral ( "Check server version" );
#else
        return QCoreApplication::translate ( "CServerDlg", "Check server version" );
#endif

    case SRS_REGISTERED:
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
        return QStringLiteral ( "Registered" );
#else
        return QCoreApplication::translate ( "CServerDlg", "Registered" );
#endif

    case SRS_SERVER_LIST_FULL:
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
        return QStringLiteral ( "Server list full at directory" );
#else
        return QCoreApplication::translate ( "CServerDlg", "Server list full at directory" );
#endif

    case SRS_VERSION_TOO_OLD:
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
        return QStringLiteral ( "Your server version is too old" );
#else
        return QCoreApplication::translate ( "CServerDlg", "Your server version is too old" );
#endif

    case SRS_NOT_FULFILL_REQUIREMENTS:
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
        return QStringLiteral ( "Requirements not fulfilled" );
#else
        return QCoreApplication::translate ( "CServerDlg", "Requirements not fulfilled" );
#endif
    }

#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
    return QStringLiteral ( "Unknown value %1" ).arg ( eSvrRegStatus );
#else
    return QString ( QCoreApplication::translate ( "CServerDlg", "Unknown value %1" ) ).arg ( eSvrRegStatus );
#endif
}

QString GetVersionAndNameStr ( const bool bDisplayInGui )
{
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
    juce::String versionText;

    if ( bDisplayInGui )
    {
        versionText += "<b>";
    }
    else
    {
#    ifdef _WIN32
        versionText += "\n";
#    endif
        versionText += " *** ";
    }

    versionText += juce::String ( APP_NAME ) + ", Version " + juce::String ( VERSION );

    if ( bDisplayInGui )
    {
        versionText += "</b><br>";
    }
    else
    {
        versionText += "\n *** ";
    }

    if ( !bDisplayInGui )
    {
        versionText += "Internet Jam Session Software";
        versionText += "\n *** ";
        versionText += "Released under the GNU General Public License version 2 or later (GPLv2)";
        versionText += "\n *** <https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>";
        versionText += "\n *** ";
        versionText += "\n *** This program is free software; you can redistribute it and/or modify it under";
        versionText += "\n *** the terms of the GNU General Public License as published by the Free Software";
        versionText += "\n *** Foundation; either version 2 of the License, or (at your option) any later version.";
        versionText += "\n *** There is NO WARRANTY, to the extent permitted by law.";
        versionText += "\n *** ";
        versionText += "\n *** This app uses the following libraries, resources or code snippets:";
        versionText += "\n *** ";
        versionText += "\n *** Qt cross-platform application framework";
        versionText += "\n *** <https://www.qt.io>";
        versionText += "\n *** ";
        versionText += "\n *** Opus Interactive Audio Codec";
        versionText += "\n *** <https://www.opus-codec.org>";
        versionText += "\n *** ";
#    ifndef SERVER_ONLY
#        if defined( _WIN32 ) && !defined( WITH_JACK )
        versionText += "\n *** ASIO (Audio Stream I/O) SDK";
        versionText += "\n *** <https://www.steinberg.net/developers>";
        versionText += "\n *** ASIO is a trademark and software of Steinberg Media Technologies GmbH";
        versionText += "\n *** ";
#        endif
#    endif
        versionText += "\n *** Copyright © 2005-2025 The Jamulus Development Team";
        versionText += "\n";
    }

    return QString::fromUtf8 ( versionText.toRawUTF8() );
#else
    QString strVersionText = "";

    if ( bDisplayInGui )
    {
        strVersionText += "<b>";
    }
    else
    {
#    ifdef _WIN32
        strVersionText += "\n";
#    endif
        strVersionText += " *** ";
    }

    strVersionText += QCoreApplication::tr ( "%1, Version %2", "%1 is app name, %2 is version number" ).arg ( APP_NAME ).arg ( VERSION );

    if ( bDisplayInGui )
    {
        strVersionText += "</b><br>";
    }
    else
    {
        strVersionText += "\n *** ";
    }

    if ( !bDisplayInGui )
    {
        strVersionText += "Internet Jam Session Software";
        strVersionText += "\n *** ";
    }

    strVersionText += QCoreApplication::tr ( "Released under the GNU General Public License version 2 or later (GPLv2)" );

    if ( !bDisplayInGui )
    {
        strVersionText += "\n *** <https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>";
        strVersionText += "\n *** ";
        strVersionText += "\n *** This program is free software; you can redistribute it and/or modify it under";
        strVersionText += "\n *** the terms of the GNU General Public License as published by the Free Software";
        strVersionText += "\n *** Foundation; either version 2 of the License, or (at your option) any later version.";
        strVersionText += "\n *** There is NO WARRANTY, to the extent permitted by law.";
        strVersionText += "\n *** ";

        strVersionText += "\n *** " + QCoreApplication::tr ( "This app uses the following libraries, resources or code snippets:" );
        strVersionText += "\n *** ";
        strVersionText += "\n *** " + QCoreApplication::tr ( "Qt cross-platform application framework" ) + QString ( " %1 " ).arg ( QT_VERSION_STR ) +
                          QCoreApplication::tr ( "(build)" ) + QString ( ", %1 " ).arg ( qVersion() ) + QCoreApplication::tr ( "(runtime)" );
        strVersionText += "\n *** <https://www.qt.io>";
        strVersionText += "\n *** ";
        strVersionText += "\n *** Opus Interactive Audio Codec";
        strVersionText += "\n *** <https://www.opus-codec.org>";
        strVersionText += "\n *** ";
#    ifndef SERVER_ONLY
#        if defined( _WIN32 ) && !defined( WITH_JACK )
        strVersionText += "\n *** ASIO (Audio Stream I/O) SDK";
        strVersionText += "\n *** <https://www.steinberg.net/developers>";
        strVersionText += "\n *** ASIO is a trademark and software of Steinberg Media Technologies GmbH";
        strVersionText += "\n *** ";
#        endif
#        ifndef HEADLESS
        strVersionText += "\n *** " + QCoreApplication::tr ( "Audio reverberation code by Perry R. Cook and Gary P. Scavone" ) +
                          ", 1995 - 2021, The Synthesis ToolKit in C++ (STK)";
        strVersionText += "\n *** <https://ccrma.stanford.edu/software/stk>";
        strVersionText += "\n *** ";
        strVersionText += "\n *** " + QString ( QCoreApplication::tr ( "Some pixmaps are from the %1" ) ).arg ( " Open Clip Art Library (OCAL)" );
        strVersionText += "\n *** <https://openclipart.org>";
        strVersionText += "\n *** ";
        strVersionText += "\n *** " + QCoreApplication::tr ( "Flag icons by Mark James" );
        strVersionText += "\n *** <http://www.famfamfam.com>";
        strVersionText += "\n *** ";
        strVersionText += "\n *** " + QString ( QCoreApplication::tr ( "Some sound samples are from %1" ) ).arg ( "Freesound" );
        strVersionText += "\n *** <https://freesound.org>";
        strVersionText += "\n *** ";
#        endif
#    endif
        strVersionText += "\n *** Copyright © 2005-2025 The Jamulus Development Team";
        strVersionText += "\n";
    }

    return strVersionText;
#endif
}

QString MakeClientNameTitle ( QString win, QString client )
{
    QString sReturnString = win;
    if ( !client.isEmpty() )
    {
        sReturnString += " - " + client;
    }
    return ( sReturnString );
}

QString TruncateString ( QString str, int position )
{
#if defined( HEADLESS ) || defined( JAMULUS_USE_JUCE_NET )
    if ( position <= 0 )
    {
        return "";
    }
    if ( position >= str.length() )
    {
        return str;
    }
    return str.left ( position );
#else
    QTextBoundaryFinder tbfString ( QTextBoundaryFinder::Grapheme, str );

    tbfString.setPosition ( position );
    if ( !tbfString.isAtBoundary() )
    {
        tbfString.toPreviousBoundary();
        position = tbfString.position();
    }
    return str.left ( position );
#endif
}
