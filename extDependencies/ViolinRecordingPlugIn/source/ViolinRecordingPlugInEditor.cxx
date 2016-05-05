#include "ViolinRecordingPlugInEditor.hxx"

#include "ComputeDescriptors.hxx"
#include "HorizontalBarMeter.hxx"

const int gridUnitPx = 5;

ViolinRecordingPlugInEditor::ViolinRecordingPlugInEditor(ViolinRecordingPlugInEffect *const ownerFilter)
    : AudioProcessorEditor(ownerFilter)
{
	LOG_INFO_N("violin_recording_plugin", "Creating editor...");

	LookAndFeel::getDefaultLookAndFeel().setColour(TextButton::buttonColourId, Colour::greyLevel(0.8f));
	LookAndFeel::getDefaultLookAndFeel().setColour(ComboBox::buttonColourId, Colour::greyLevel(0.8f));
	LookAndFeel::getDefaultLookAndFeel().setColour(Label::textColourId, Colour(0xff000000));

	// -----------------------------------------------------------------------------------

#if (ENABLE_CHECK_PROCESSING_DEAD != 0)
	isEffectProcessingSuspended_ = true;
#else
	isEffectProcessingSuspended_ = false;
#endif
	arePlotGraphsValid_.set(false);
	isScene3dValid_.set(false);

	// -----------------------------------------------------------------------------------

	setSize(0, 0); // true size set later later (depending on config)

	int x;
	int y;

	// -----------------------------------------------------------------------------------

	x = 2;
	y = 3;

	// Configuration label:
	Label *configLabel = new Label(String::empty, T("Configuration:"));
	addAndMakeVisible(configLabel);
	configLabel->setBounds(x*gridUnitPx, (y+1)*gridUnitPx, 20*gridUnitPx, 2*gridUnitPx);

	// Use gages toggle:
	addAndMakeVisible(useGagesButton_ = new ToggleButton(T("Use gages")));
	useGagesButton_->setBounds((x+20+2)*gridUnitPx, y*gridUnitPx, 16*gridUnitPx, 4*gridUnitPx);
	useGagesButton_->setTooltip(T("Use bow force as measured from strain gages (over serial comm. port)."));
	useGagesButton_->addButtonListener(this);
	useGagesButton_->setMouseClickGrabsKeyboardFocus(false);
	// default value set on updateConfigurationSection()

	// COM port selector combo box:
	addAndMakeVisible(comPortComboBox_ = new ComboBox(T("com port combo box")));
	comPortComboBox_->clear();
	for (int i = 0; i < ComPort::getNumComPorts(); ++i)
	{
		comPortComboBox_->addItem(ComPort::getPortName(i), i+1);
	}
	comPortComboBox_->setBounds((x + 20+2 + 16)*gridUnitPx, y*gridUnitPx, 12*gridUnitPx, 4*gridUnitPx);
	comPortComboBox_->addListener(this);
	// default value set on updateConfigurationSection()

	// Use stereo toggle:
	addAndMakeVisible(useStereoButton_ = new ToggleButton(T("Use 2ch. in")));
	useStereoButton_->setBounds((x + 20+2 + 16+12+4)*gridUnitPx, y*gridUnitPx, 16*gridUnitPx, 4*gridUnitPx);	
	useStereoButton_->setTooltip(T("Use two channel input for recording, else only left channel will be used."));
	useStereoButton_->addButtonListener(this);
	useStereoButton_->setMouseClickGrabsKeyboardFocus(false);
	// default value set on updateConfigurationSection()

	// Use automatic string estimation toggle:
	addAndMakeVisible(useAutoStringButton_ = new ToggleButton(T("Auto string")));
	useAutoStringButton_->setBounds((x + 20+2 + 16+12+4 + 16+4)*gridUnitPx, y*gridUnitPx, 17*gridUnitPx, 4*gridUnitPx);
	useAutoStringButton_->setTooltip(T("Use automatic played string estimation (needs angles file), else string 1 (right) is used."));
	useAutoStringButton_->addButtonListener(this);
	useAutoStringButton_->setMouseClickGrabsKeyboardFocus(false);
	// default value set on updateConfigurationSection()

	// Use scene 3d toggle:
	addAndMakeVisible(useScene3dButton_ = new ToggleButton(T("Use 3d scene")));
	useScene3dButton_->setBounds((x + 20+2 + 16+12+4 + 16+4 + 17+4)*gridUnitPx, y*gridUnitPx, 17*gridUnitPx, 4*gridUnitPx);
	useScene3dButton_->setTooltip(T("Enable real-time drawing of 3d scene representing sensors in space (resizes gui)."));
	useScene3dButton_->addButtonListener(this);
	useScene3dButton_->setMouseClickGrabsKeyboardFocus(false);

	// -----------------------------------------------------------------------------------

	x = 2;
	y += 4+1;

	// Tracker label:
	Label *trackerLabel = new Label(String::empty, T("Tracker:"));
	addAndMakeVisible(trackerLabel);
	trackerLabel->setBounds(x*gridUnitPx, (y+1)*gridUnitPx, 20*gridUnitPx, 2*gridUnitPx);

	// Connect/disconnect buttons:
	addAndMakeVisible(connectTrackerButton_ = new TextButton(T("Connect")));
	connectTrackerButton_->addButtonListener(this);
	connectTrackerButton_->setColour(TextButton::buttonColourId, Colour(0xffa9f30e));
	connectTrackerButton_->setConnectedEdges(Button::ConnectedOnRight);
	connectTrackerButton_->setBounds((x+20+2)*gridUnitPx, y*gridUnitPx, 20*gridUnitPx, 4*gridUnitPx);

	addAndMakeVisible(disconnectTrackerButton_ = new TextButton(T("Disconnect")));
	disconnectTrackerButton_->addButtonListener(this);
	disconnectTrackerButton_->setColour(TextButton::buttonColourId, Colour(0xffff511b));
	disconnectTrackerButton_->setConnectedEdges(Button::ConnectedOnLeft);
	disconnectTrackerButton_->setBounds((x+20+2+20)*gridUnitPx, y*gridUnitPx, 20*gridUnitPx, 4*gridUnitPx);

	// Tracker status label:
	trackerStatusLabel_ = new Label(String::empty, T("")); // (text will be set later)
	addAndMakeVisible(trackerStatusLabel_);
	trackerStatusLabel_->setBounds((x+20+2+20+20+6)*gridUnitPx, y*gridUnitPx, 60*gridUnitPx, 2*gridUnitPx);

	// Calibration status label:
	calibrationStatusLabel_ = new Label(String::empty, T("")); // (text will be set label)
	addAndMakeVisible(calibrationStatusLabel_);
	calibrationStatusLabel_->setBounds((x+20+2+20+20+6)*gridUnitPx, (y+2)*gridUnitPx, 60*gridUnitPx, 2*gridUnitPx);

	// -----------------------------------------------------------------------------------

	x = 2;
	y += 4+1;

	// Calibration:
	Label *calibrationLabel = new Label(String::empty, T("Calibration:"));
	addAndMakeVisible(calibrationLabel);
	calibrationLabel->setBounds(x*gridUnitPx, (y+1)*gridUnitPx, 20*gridUnitPx, 2*gridUnitPx);

	addAndMakeVisible(calibrateButton_ = new TextButton(T("Calib. tracker")));
	calibrateButton_->addButtonListener(this);
	calibrateButton_->setBounds((x+20+2)*gridUnitPx, y*gridUnitPx, 20*gridUnitPx, 4*gridUnitPx);

	addAndMakeVisible(loadCalibrationButton_ = new TextButton(T("Load...")));
	loadCalibrationButton_->addButtonListener(this);
	loadCalibrationButton_->setBounds((x+20+2+20+2)*gridUnitPx, y*gridUnitPx, 19*gridUnitPx, 4*gridUnitPx);

	addAndMakeVisible(saveCalibrationButton_ = new TextButton(T("Save as...")));
	saveCalibrationButton_->addButtonListener(this);
	saveCalibrationButton_->setBounds((x+20+2+20+2+19+2)*gridUnitPx, y*gridUnitPx, 19*gridUnitPx, 4*gridUnitPx);

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	addAndMakeVisible(calibrateForceButton_ = new TextButton(T(""))); // (text will be set later)
	calibrateForceButton_->addButtonListener(this);
	calibrateForceButton_->setBounds((x+20+2+20+2+19+2+19+2+2+1)*gridUnitPx, y*gridUnitPx, 30*gridUnitPx, 4*gridUnitPx);

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	addAndMakeVisible(calibrationTakeSampleButton_ = new TextButton(T("Take sample")));
	calibrationTakeSampleButton_->addButtonListener(this);
	calibrationTakeSampleButton_->setBounds((x+20+2)*gridUnitPx, y*gridUnitPx, 20*gridUnitPx, 4*gridUnitPx);
	calibrationTakeSampleButton_->setVisible(false);

	addAndMakeVisible(calibrationPrevStepButton_ = new TextButton(T("Prev. step")));
	calibrationPrevStepButton_->addButtonListener(this);
	calibrationPrevStepButton_->setBounds((x+20+2+20+2)*gridUnitPx, y*gridUnitPx, 20*gridUnitPx, 4*gridUnitPx);
	calibrationPrevStepButton_->setVisible(false);

	addAndMakeVisible(calibrationNextStepButton_ = new TextButton(T("Next step")));
	calibrationNextStepButton_->addButtonListener(this);
	calibrationNextStepButton_->setBounds((x+20+2+20+2+20+2)*gridUnitPx, y*gridUnitPx, 20*gridUnitPx, 4*gridUnitPx);
	calibrationNextStepButton_->setVisible(false);

	addAndMakeVisible(calibrationStopButton_ = new TextButton(T("Stop")));
	calibrationStopButton_->addButtonListener(this);
	calibrationStopButton_->setBounds((x+20+2+20+2+20+2+20+2)*gridUnitPx, y*gridUnitPx, 20*gridUnitPx, 4*gridUnitPx);
	calibrationStopButton_->setVisible(false);

	// -----------------------------------------------------------------------------------

	x = 2;
	y += 4+1;

	// Score list:
	Label *scoreListChooserLabel = new Label(String::empty, T("Score List:"));
	addAndMakeVisible(scoreListChooserLabel);
	scoreListChooserLabel->setBounds(x*gridUnitPx, (y+1)*gridUnitPx, 20*gridUnitPx, 2*gridUnitPx);

	addAndMakeVisible(scoreListFileChooser_ = new FilenameComponent(T("score list file chooser"), File::nonexistent, false, false, false, T("*.txt;*.scoreList"), String::empty, T("(choose score list file)")));
	scoreListFileChooser_->addListener(this);
	scoreListFileChooser_->setBrowseButtonText(T("Browse..."));
	scoreListFileChooser_->setBounds((x+20+2)*gridUnitPx, y*gridUnitPx, 86*gridUnitPx, 4*gridUnitPx);

	// Output path:
	Label *outputPathChooserLabel = new Label(String::empty, T("Output Path:"));
	addAndMakeVisible(outputPathChooserLabel);
	outputPathChooserLabel->setBounds(x*gridUnitPx, (y+4+1+1)*gridUnitPx, 20*gridUnitPx, 2*gridUnitPx);

	addAndMakeVisible(outputPathChooser_ = new FilenameComponent(T("ouput path chooser"), File::nonexistent, false, true, false, String::empty, String::empty, T("(choose output path)")));
	outputPathChooser_->addListener(this);
	outputPathChooser_->setBrowseButtonText(T("Browse..."));
	outputPathChooser_->setBounds((x+20+2)*gridUnitPx, (y+4+1)*gridUnitPx, 86*gridUnitPx, 4*gridUnitPx);

	// -----------------------------------------------------------------------------------

	x = 2;
	y += 2*(4+1);

	// Messages label:
	Label *trackerLogLabel = new Label(String::empty, T("Messages:"));
	addAndMakeVisible(trackerLogLabel);
	trackerLogLabel->setBounds(x*gridUnitPx, (y+1)*gridUnitPx, 20*gridUnitPx, 2*gridUnitPx);

	// Clear event log display button:
	addAndMakeVisible(clearEventLogButton_ = new TextButton(T("Clear")));
	clearEventLogButton_->addButtonListener(this);
	clearEventLogButton_->setBounds(x*gridUnitPx, (y+4)*gridUnitPx, 14*gridUnitPx, 4*gridUnitPx);
	clearEventLogButton_->setEnabled(false);

	// Messages (events) list box:
	addAndMakeVisible(trackerLogListBox_ = new ListBox(T("messages list box"), this));
	trackerLogListBox_->setRowHeight(2*gridUnitPx);
	trackerLogListBox_->selectRow(0);
	trackerLogListBox_->setColour(ListBox::outlineColourId, Colours::black);
	trackerLogListBox_->setOutlineThickness(1);
	trackerLogListBox_->setBounds((x+20+2)*gridUnitPx, y*gridUnitPx, 86*gridUnitPx, 12*2*gridUnitPx);

	// Buffer meters:
	Label *bufferUsageLabel = new Label(String::empty, T("Buffer usage:"));
	addAndMakeVisible(bufferUsageLabel);
	bufferUsageLabel->setBounds(x*gridUnitPx, (y+12*2-8)*gridUnitPx, 20*gridUnitPx, 2*gridUnitPx);

	HorizontalBarMeter *audioCh1BufferFullnessMeter = new HorizontalBarMeter(T("audio ch1 buffer fullness meter"));
	addAndMakeVisible(audioCh1BufferFullnessMeter);
	audioCh1BufferFullnessMeter->setBounds(x*gridUnitPx, (y+12*2-5)*gridUnitPx, 20*gridUnitPx, 1*gridUnitPx);
	audioCh1BufferFullnessMeter->setTooltip(T("Fullness of buffer for audio channel one."));
	getFilter()->setAudioCh1BufferFullnessMeter(audioCh1BufferFullnessMeter);

	HorizontalBarMeter *audioCh2BufferFullnessMeter = new HorizontalBarMeter(T("audio ch2 buffer fullness meter"));
	addAndMakeVisible(audioCh2BufferFullnessMeter);
	audioCh2BufferFullnessMeter->setBounds(x*gridUnitPx, (y+12*2-4)*gridUnitPx-1, 20*gridUnitPx, 1*gridUnitPx);
	audioCh2BufferFullnessMeter->setTooltip(T("Fullness of buffer for audio channel two."));
	getFilter()->setAudioCh2BufferFullnessMeter(audioCh2BufferFullnessMeter);

	HorizontalBarMeter *trackerBufferFullnessMeter = new HorizontalBarMeter(T("tracker buffer fullness meter"));
	addAndMakeVisible(trackerBufferFullnessMeter);
	trackerBufferFullnessMeter->setBounds(x*gridUnitPx, (y+12*2-2)*gridUnitPx, 20*gridUnitPx, 1*gridUnitPx);
	trackerBufferFullnessMeter->setTooltip(T("Fullness of tracker buffer."));
	trackerBufferFullnessMeter->setBarColour(Colours::dodgerblue);
	getFilter()->setTrackerBufferFullnessMeter(trackerBufferFullnessMeter);

	HorizontalBarMeter *arduinoBufferFullnessMeter = new HorizontalBarMeter(T("arduino buffer fullness meter"));
	addAndMakeVisible(arduinoBufferFullnessMeter);
	arduinoBufferFullnessMeter->setBounds(x*gridUnitPx, (y+12*2-1)*gridUnitPx-1, 20*gridUnitPx, 1*gridUnitPx);
	arduinoBufferFullnessMeter->setTooltip(T("Fullness of arduino buffer."));
	arduinoBufferFullnessMeter->setBarColour(Colours::cadetblue);
	getFilter()->setArduinoBufferFullnessMeter(arduinoBufferFullnessMeter);

	// -----------------------------------------------------------------------------------

	x = 2;
	y += 12*2+2;

	// Next/recording phrase label:
	phraseLabel_ = new Label(String::empty, T("")); // (text will be set later)
	addAndMakeVisible(phraseLabel_);
	phraseLabel_->setBounds(x*gridUnitPx, (y+1)*gridUnitPx, 24*gridUnitPx, 2*gridUnitPx);

	// Phrase index label:
	phraseIndexLabel_ = new Label(String::empty, T("")); // (text will be set later)
	addAndMakeVisible(phraseIndexLabel_);
	phraseIndexLabel_->setBounds((x+24+1)*gridUnitPx, (y+1)*gridUnitPx, 6*gridUnitPx, 2*gridUnitPx);

	// Phrase name (filename) text editor:
	phraseNameEditor_ = new TextEditor();
	addAndMakeVisible(phraseNameEditor_);
	phraseNameEditor_->addListener(this);
	phraseNameEditor_->setBounds((x+24+1+6+1)*gridUnitPx, y*gridUnitPx, 65*gridUnitPx, 4*gridUnitPx);
	phraseNameEditor_->setText(T("")); // (text will be set later)

	// Take index label:
	takeIndexLabel_ = new Label(String::empty, T("")); // (text will be set later)
	addAndMakeVisible(takeIndexLabel_);
	takeIndexLabel_->setBounds((x+24+1+6+1+65+1)*gridUnitPx, (y+1)*gridUnitPx, 12*gridUnitPx, 2*gridUnitPx);

	// Previous phrase button:
	addAndMakeVisible(prevPhraseButton_ = new TextButton(T("<<")));
	prevPhraseButton_->addButtonListener(this);
	prevPhraseButton_->setConnectedEdges(Button::ConnectedOnRight);
	prevPhraseButton_->setBounds((x+24+1+6+1+65+1+12+1)*gridUnitPx, y*gridUnitPx, 5*gridUnitPx, 4*gridUnitPx);

	// Next phrase button:
	addAndMakeVisible(nextPhraseButton_ = new TextButton(T(">>")));
	nextPhraseButton_->addButtonListener(this);
	nextPhraseButton_->setConnectedEdges(Button::ConnectedOnLeft);
	nextPhraseButton_->setBounds((x+24+1+6+1+65+1+12+1+5)*gridUnitPx, y*gridUnitPx, 5*gridUnitPx, 4*gridUnitPx);

	// -----------------------------------------------------------------------------------

	x = 2;
	y += 4+2;

	// Bow displacement:
	Label *bowDisplacementLabel = new Label(String::empty, T("Bow Displacement (0 .. 70 cm):"));
	addAndMakeVisible(bowDisplacementLabel);
	bowDisplacementLabel->setBounds(x*gridUnitPx, y*gridUnitPx, 60*gridUnitPx, 2*gridUnitPx);

	bowDisplacementPlot_ = new GraphPlotter("bow displacement plot");
	addAndMakeVisible(bowDisplacementPlot_);
	bowDisplacementPlot_->setYLim(0.0f, 70.0f);
	bowDisplacementPlot_->setBounds(x*gridUnitPx, (y+3)*gridUnitPx, 60*gridUnitPx, 25*gridUnitPx);
	bowDisplacementPlot_->setHorizontalOverlay(65.4f, true);
	bowDisplacementPlot_->setScrolling(true);

	// Bow force:
	bowForceLabel_ = new Label(String::empty, T("")); // will be set later
	addAndMakeVisible(bowForceLabel_);
	bowForceLabel_->setBounds((x+61)*gridUnitPx, y*gridUnitPx, 60*gridUnitPx, 2*gridUnitPx);

	bowForcePlot_ = new GraphPlotter("bow force plot");
	addAndMakeVisible(bowForcePlot_);
	bowForcePlot_->setYLim(0.0f, 100.0f); // will be overridden later (but just in case)
	bowForcePlot_->setBounds((x+61)*gridUnitPx, (y+3)*gridUnitPx, 60*gridUnitPx, 40*gridUnitPx);
	bowForcePlot_->setNumValuesPerPixel(1);
	bowForcePlot_->setScrolling(true);

	// Bow-bridge distance:
	Label *bowBridgeDistanceLabel = new Label(String::empty, T("Bow-Bridge Distance (0 .. 10 cm, abs):"));
	addAndMakeVisible(bowBridgeDistanceLabel);
	bowBridgeDistanceLabel->setBounds(x*gridUnitPx, (y+3+25+1)*gridUnitPx, 91*gridUnitPx, 2*gridUnitPx);

	bowBridgeDistancePlot_ = new GraphPlotter("bow-bridge distance plot");
	addAndMakeVisible(bowBridgeDistancePlot_);
	bowBridgeDistancePlot_->setYLim(0.0f, 10.0f);
	bowBridgeDistancePlot_->setBounds(x*gridUnitPx, (y+3+25+1+3)*gridUnitPx, 60*gridUnitPx, 25*gridUnitPx);
	bowBridgeDistancePlot_->setHorizontalOverlay(5.7f, true);
	bowBridgeDistancePlot_->setScrolling(true);

	// Sensor acceleration:
	summedSensorAccelerationLabel_ = new Label(String::empty, T("")); // will be set later
	addAndMakeVisible(summedSensorAccelerationLabel_);
	summedSensorAccelerationLabel_->setBounds((x+61)*gridUnitPx, (y+3+25+1+15)*gridUnitPx, 60*gridUnitPx, 2*gridUnitPx);

	noiseAndSyncPlot_ = new GraphPlotter("tracker noise and stream sync plot");
	addAndMakeVisible(noiseAndSyncPlot_);
	noiseAndSyncPlot_->setYLim(0.0f, 10.0f); // arbitrary scale for most plots, cm for stick-bridge distance
//	noiseAndSyncPlot_->setBounds((x+61)*gridUnitPx, (y+3+25+1+3)*gridUnitPx, 60*gridUnitPx, 25*gridUnitPx);
	noiseAndSyncPlot_->setBounds((x+61)*gridUnitPx, (y+3+25+1+3+15)*gridUnitPx, 60*gridUnitPx, 10*gridUnitPx);
	noiseAndSyncPlot_->addPlot(Colours::green); // input audio signal (for stream synchronization)
	noiseAndSyncPlot_->setNumValuesPerPixel(1);
	noiseAndSyncPlot_->setScrolling(true);

	arePlotGraphsValid_.set(true);

	// -----------------------------------------------------------------------------------

	x = 2;
	y += 3+25+1+3+25;

	addAndMakeVisible(manualSyncButton_ = new ToggleButton(T("Manual sync")));
	manualSyncButton_->setBounds(x*gridUnitPx, (y+2)*gridUnitPx, 17*gridUnitPx, 4*gridUnitPx);
	manualSyncButton_->setTooltip(T("Enable/disable manually entered stream offsets rather than estimated stream offsets obtained by sending and receiving sync signals."));
	manualSyncButton_->addButtonListener(this);
	manualSyncButton_->setMouseClickGrabsKeyboardFocus(false);
	// default value set on updateSyncSection()

	x += 17+1;

	String trackerToAudioSyncOffsetTooltip = T("Adjust synchronization offset of tracker stream relative to audio stream. Negative means advance tracker, positive means delay tracker.");
	Label *trackerToAudioSyncOffsetLabel = new Label(String::empty, T("Tr./Aud.:"));
	addAndMakeVisible(trackerToAudioSyncOffsetLabel);
	trackerToAudioSyncOffsetLabel->setBounds(x*gridUnitPx, (y+3)*gridUnitPx, 11*gridUnitPx, 2*gridUnitPx);
	trackerToAudioSyncOffsetLabel->setTooltip(trackerToAudioSyncOffsetTooltip);
	addAndMakeVisible(trackerToAudioSyncOffsetSlider_ = new Slider(T("tracker to audio sync offset slider")));
	trackerToAudioSyncOffsetSlider_->setSliderStyle(Slider::IncDecButtons);
	trackerToAudioSyncOffsetSlider_->setTextBoxStyle(Slider::TextBoxLeft, false, 11*gridUnitPx, 4*gridUnitPx);
	trackerToAudioSyncOffsetSlider_->setBounds((x+11+1)*gridUnitPx, (y+2)*gridUnitPx, 2*11*gridUnitPx, 4*gridUnitPx);//(4+3)*gridUnitPx+4); // + 4 because 2px margin/border around top/bottom buttons
	trackerToAudioSyncOffsetSlider_->setIncDecButtonsMode(Slider::incDecButtonsDraggable_Vertical);
	trackerToAudioSyncOffsetSlider_->setRange(-720.0, 720.0, 1.0);
	trackerToAudioSyncOffsetSlider_->setTooltip(trackerToAudioSyncOffsetTooltip);
	trackerToAudioSyncOffsetSlider_->setChangeNotificationOnlyOnRelease(true);
	trackerToAudioSyncOffsetSlider_->addListener(this);
	// Note: tooltip only works when hovering the mouse in between the buttons and the textbox

	String arduinoToTrackerSyncOffsetTooltip = T("Adjust synchronization offset of Arduino stream relative to tracker stream. Negative means advance Arduino, positive means delay Arduino.");
	Label *arduinoToTrackerSyncOffsetLabel = new Label(String::empty, T("Ard./Tr.:"));
	addAndMakeVisible(arduinoToTrackerSyncOffsetLabel);
	arduinoToTrackerSyncOffsetLabel->setBounds(x*gridUnitPx, (y+3+4+1)*gridUnitPx, 11*gridUnitPx, 2*gridUnitPx);
	arduinoToTrackerSyncOffsetLabel->setTooltip(arduinoToTrackerSyncOffsetTooltip);
	addAndMakeVisible(arduinoToTrackerSyncOffsetSlider_ = new Slider(T("arduino to tracker sync offset slider")));
	arduinoToTrackerSyncOffsetSlider_->setSliderStyle(Slider::IncDecButtons);
	arduinoToTrackerSyncOffsetSlider_->setTextBoxStyle(Slider::TextBoxLeft, false, 11*gridUnitPx, 4*gridUnitPx);
	arduinoToTrackerSyncOffsetSlider_->setBounds((x+11+1)*gridUnitPx, (y+2+4+1)*gridUnitPx, 2*11*gridUnitPx, 4*gridUnitPx);//(4+3)*gridUnitPx+4); // + 4 because 2px margin/border around top/bottom buttons
	arduinoToTrackerSyncOffsetSlider_->setIncDecButtonsMode(Slider::incDecButtonsDraggable_Vertical);
	arduinoToTrackerSyncOffsetSlider_->setRange(-720.0, 720.0, 1.0);
	arduinoToTrackerSyncOffsetSlider_->setTooltip(arduinoToTrackerSyncOffsetTooltip);
	arduinoToTrackerSyncOffsetSlider_->setChangeNotificationOnlyOnRelease(true);
	arduinoToTrackerSyncOffsetSlider_->addListener(this);
	// Note: tooltip only works when hovering the mouse in between the buttons and the textbox

	x += 11+1+11+2+11+1+11+2;
	x -= 8;

	addAndMakeVisible(plotSyncSigButton_ = new ToggleButton(T("Plot sync sig.")));
	plotSyncSigButton_->setBounds(x*gridUnitPx, (y+2)*gridUnitPx, 17*gridUnitPx, 4*gridUnitPx);
	plotSyncSigButton_->setTooltip(T("Plot sync signals (mainly to check if cables work correctly, etc.)."));
	plotSyncSigButton_->addButtonListener(this);
	plotSyncSigButton_->setMouseClickGrabsKeyboardFocus(false);
	// default value set on updateSyncSection()

//t	addAndMakeVisible(contRetriggerButton_ = new ToggleButton(T("Cont. retrigger")));
//t	contRetriggerButton_->setBounds(x*gridUnitPx, (y+2+4)*gridUnitPx, 17*gridUnitPx, 4*gridUnitPx);
//t	contRetriggerButton_->setTooltip(T("Continuously resend output sync signal and adjust stream offsets accordingly."));
//t	contRetriggerButton_->addButtonListener(this);
//t	contRetriggerButton_->setMouseClickGrabsKeyboardFocus(false);
//t	// default value set on updateSyncSection()

	addAndMakeVisible(triggerSyncButton_ = new TextButton(T("Trigger sync")));
	triggerSyncButton_->addButtonListener(this);
	triggerSyncButton_->setBounds((x+17+2)*gridUnitPx, (y+4)*gridUnitPx, 19*gridUnitPx, 4*gridUnitPx);

	x += 8;
	x -= 11+1+11+2+11+1+11+2;
	x -= 17+1;

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	y -= 2*(4+1);
	y += 12;
	Label *tuningRefToneLabel = new Label(String::empty, T("Tune:"));
	addAndMakeVisible(tuningRefToneLabel);
	tuningRefToneLabel->setBounds((x+20+2+86+2)*gridUnitPx, y*gridUnitPx, 11*gridUnitPx, 2*gridUnitPx);

	addAndMakeVisible(tuningRefToneButton_ = new ToggleButton(T("442 Hz")));
	tuningRefToneButton_->addButtonListener(this);
	tuningRefToneButton_->setBounds((x+20+2+86+2)*gridUnitPx, (y+2+1)*gridUnitPx, 11*gridUnitPx, 4*gridUnitPx);
	tuningRefToneButton_->setTooltip(T("Sends a 442 Hz (A) reference tone over output 1 and 2 for tuning violin. Input monitoring should be turned on for the channel to which these outputs are route."));
	tuningRefToneButton_->setMouseClickGrabsKeyboardFocus(false);
	y += 2*(4+1);
	y -= 12;

	// -----------------------------------------------------------------------------------

	Label *scene3dLabel = new Label(String::empty, T("3D Scene:"));
	addAndMakeVisible(scene3dLabel);
	scene3dLabel->setBounds(125*gridUnitPx, (2+1)*gridUnitPx, 60*gridUnitPx, 2*gridUnitPx);

	scene3d_ = NULL;
	// scene3d_ already created on updateScene3dSection()

	addAndMakeVisible(makeModelButton_ = new TextButton(T("Make model")));
	makeModelButton_->addButtonListener(this);
	makeModelButton_->setBounds(125*gridUnitPx, (2 + 4 + 105 + 2)*gridUnitPx, 20*gridUnitPx, 4*gridUnitPx);

	addAndMakeVisible(loadModelButton_ = new TextButton(T("Load model")));
	loadModelButton_->addButtonListener(this);
	loadModelButton_->setBounds((125+20+2)*gridUnitPx, (2 + 4 + 105 + 2)*gridUnitPx, 20*gridUnitPx, 4*gridUnitPx);

	addAndMakeVisible(resetCameraButton_ = new TextButton(T("Reset camera")));
	resetCameraButton_->addButtonListener(this);
	resetCameraButton_->setBounds((125+105-20)*gridUnitPx, (2 + 4 + 105 + 2)*gridUnitPx, 20*gridUnitPx, 4*gridUnitPx);

	// -----------------------------------------------------------------------------------

//	// Set focus to non-ToggleButton widget:
//	connectTrackerButton_->grabKeyboardFocus(); // XXX: doesn't work

	// Connect signals and slots between editor and effect:
	updateEntireEditorSlot_.bindAndConnect(getFilter()->updateEntireEditorSignal, this, &ViolinRecordingPlugInEditor::updateEntireEditor);
	updateLoggerListBoxAsynchronouslySlot_.bindAndConnect(getFilter()->updateLoggerListBoxAsynchronousSignal, this, &ViolinRecordingPlugInEditor::updateLoggerListBoxAsynchronous);
	endTrackerCalibrationSequenceReachedSlot_.bindAndConnect(getFilter()->endTrackerCalibrationSequenceReachedSignal, this, &ViolinRecordingPlugInEditor::endTrackerCalibrationSequenceReached);
	endForceCalibrationSequenceReachedSlot_.bindAndConnect(getFilter()->endForceCalibrationSequenceReachedSignal, this, &ViolinRecordingPlugInEditor::endForceCalibrationSequenceReached);
	endSampling3dModelSequenceReachedSlot_.bindAndConnect(getFilter()->endSampling3dModelSequenceReachedSignal, this, &ViolinRecordingPlugInEditor::endSampling3dModelSequenceReached);
	endRecordingPhraseSlot_.bindAndConnect(getFilter()->endRecordingPhraseSignal, this, &ViolinRecordingPlugInEditor::doEndRecordingPhraseDialog);

	// Set using gages state:
	isUsingGages_ = -1; // unknown

	// Update entire editor (by polling effect state):
	updateEntireEditor(NULL);

	// Set up timer:
	lastProcessingDeadFlagSetTime_ = 0;
	lastCheckedScoreListModTime_ = Time(1970, 1, 1, 1, 1);
	
	startTimer(1500);
}

ViolinRecordingPlugInEditor::~ViolinRecordingPlugInEditor()
{
	// Log messages:
	LOG_INFO_N("violin_recording_plugin", "Destroying editor...");

	if (scene3d_ != NULL)
	{
		LOG_INFO_N("violin_recording_plugin", "Destroying 3d scene...");
		// (done on deleteAllChildren(), just so log is symmetric)
	}

	// Stop timer (blocking):
	stopTimer();

	// Set shared pointers to objects to NULL to stop effect from using them 
	// (objects will be deleted on deleteAllChildren() below):
	bowDisplacementPlot_ = NULL;
	bowForcePlot_ = NULL;
	bowBridgeDistancePlot_ = NULL;
	noiseAndSyncPlot_ = NULL;
	scene3d_ = NULL;

	// XXX NOT THREAD SAFE -------------------------------------------------------------->
	// (editor may be destroyed while effect is running)
	// ...:
	getFilter()->setAudioCh1BufferFullnessMeter(NULL);
	getFilter()->setAudioCh2BufferFullnessMeter(NULL);
	getFilter()->setTrackerBufferFullnessMeter(NULL);
	getFilter()->setArduinoBufferFullnessMeter(NULL);
	// <----------------------------------------------------------------------------------

	// Unbind signals/slots:
	updateEntireEditorSlot_.unbindAndDisconnect();
	updateLoggerListBoxAsynchronouslySlot_.unbindAndDisconnect();
	endTrackerCalibrationSequenceReachedSlot_.unbindAndDisconnect();
	endForceCalibrationSequenceReachedSlot_.unbindAndDisconnect();
	endSampling3dModelSequenceReachedSlot_.unbindAndDisconnect();
	endRecordingPhraseSlot_.unbindAndDisconnect();

	// Sleep a while to ensure effect isn't using shared objects anymore:
	Thread::sleep(250);

	// Delete all child Components:
	deleteAllChildren();
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEditor::filenameComponentChanged(FilenameComponent *filenameComponent)
{
	if (filenameComponent == scoreListFileChooser_)
	{
		{
			// LOCK
			const ScopedLock scopedScoreListLock(scoreListLock_);

			if (!getFilter()->setScoreListFilenameAndLoadIfValid(filenameComponent->getCurrentFile().getFullPathName()))
			{
				String msg = T("Loading score list file failed.");
				AlertWindow::showMessageBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), msg, T("Ok"));
			}
			else
			{
				// Reset external file update check (so it doesn't alert when loading new score list):
				lastCheckedScoreListModTime_ = Time(1970, 1, 1, 1, 1);

				// If score list file is valid and its path is different than the 
				// current output path, ask if the user wants to use this path as the output path:
				File dir = filenameComponent->getCurrentFile().getParentDirectory();
				if (dir.getFullPathName() != getFilter()->getOutputPath())
				{
					String msg = T("Use score list path as the output path (recommended)?");
					if (AlertWindow::showOkCancelBox(AlertWindow::QuestionIcon, T("Violin Recording Plug-In"), msg, T("Yes"), T("No")))
						getFilter()->setOutputPath(dir.getFullPathName());
				}
			}
			// UNLOCK
		}

		updateEntireEditor(NULL);
	}
	else if (filenameComponent == outputPathChooser_)
	{
		if (filenameComponent->getCurrentFile().getBytesFreeOnVolume() == 0)
		{
			String msg = T("No free diskspace on selected volume or read-only. Recording will fail.");
			AlertWindow::showMessageBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), msg, T("Ok"));
		}

		getFilter()->setOutputPath(filenameComponent->getCurrentFile().getFullPathName());

		updateEntireEditor(NULL);
	}
}

void ViolinRecordingPlugInEditor::buttonClicked(Button *button)
{
	if (button == useGagesButton_)
	{
		getFilter()->setUseGagesEnabled(button->getToggleState());
		updateEntireEditor(NULL); // enable/disable combobox, graph plots (label, y-limit)
	}
	else if (button == useStereoButton_)
	{
		getFilter()->setUseStereoEnabled(button->getToggleState());
		updateEntireEditor(NULL); // (not really needed as nothing gui is changed besides button itself)
	}
	else if (button == useAutoStringButton_)
	{
		bool changeButtonState = true;
		bool desiredButtonState = button->getToggleState();

		if (desiredButtonState == true)
		{
			button->setToggleState(false, false); // disable button until file selected, etc.

			bool openLoadFileDialog = false;

			if (!getFilter()->getAnglesCalibration().isValid())
			{
				openLoadFileDialog = true;
			}
			else
			{
				String msg = T("Open different angles calibration file? Otherwise continue using current angles calibration.");
				if (AlertWindow::showOkCancelBox(AlertWindow::QuestionIcon, T("Violin Recording Plug-In"), msg, T("Yes"), T("No")))
					openLoadFileDialog = true;
			}

			if (openLoadFileDialog)
			{
//				WildcardFileFilter wildcardFilter(T("*.csv"), T("Comma Separated Value files"));
//				FileBrowserComponent browser(FileBrowserComponent::loadFileMode, File::nonexistent, &wildcardFilter, 0, false, false);
//				FileChooserDialogBox dialogBox(T("Open angles calibration file"), T("Please choose angles calibration file to open..."), browser, false, Colour::greyLevel(0.9f));

//				if (dialogBox.show())

				FileChooser fileChooser(T("Open angles calibration file"), File::nonexistent, T("*.csv"), true);

				if (fileChooser.browseForFileToOpen())
				{
					File selectedFile = fileChooser.getResult();//browser.getCurrentFile();
					if (!getFilter()->getAnglesCalibration().loadFromFile(selectedFile.getFullPathName()))
					{
						String msg = T("Failed loading angles calibration file.");
						AlertWindow::showMessageBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), msg, T("Ok"));
						changeButtonState = false;
					}
				}
				else
				{
					changeButtonState = false;
				}
			}
		}

		if (changeButtonState)
			getFilter()->setAutoStringEnabled(desiredButtonState);

		updateEntireEditor(NULL); // update button in case going disabled->enabled and changeButtonState was false
	}
	else if (button == useScene3dButton_)
	{
//		LOG_DEBUG_N("violin_recording_plugin", "DEBUG calling setUseScene3dEnabled()");
		getFilter()->setUseScene3dEnabled(button->getToggleState());
//		LOG_DEBUG_N("violin_recording_plugin", "DEBUG calling updateEntireEditor()");
		updateEntireEditor(NULL); // update gui size, etc.
	}
	// -----------------------------------------------------------------------------------
	else if (button == connectTrackerButton_)
	{
		String msg = T("All sensors must be located in the positive-x hemisphere while connecting. Continue?");
		if (!AlertWindow::showOkCancelBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), msg, T("Yes"), T("No")))
			return;

		getFilter()->connectTrackerAsynch(); // updates gui as well
	}
	else if (button == disconnectTrackerButton_)
	{
		getFilter()->disconnectTrackerAsynch(); // updates gui as well
	}
	// -----------------------------------------------------------------------------------
	else if (button == calibrateButton_)
	{
		if (getFilter()->getTrackerCalibrationState() == ViolinRecordingPlugInEffect::TRACKER_CALIBRATED)
		{
			String msg = T("Calibrating will overwrite current calibration data. Continue?");
			if (!AlertWindow::showOkCancelBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), msg, T("Yes"), T("No")))
				return;
		}

		startTrackerCalibration();
	}
	else if (button == loadCalibrationButton_)
	{
		if (getFilter()->getTrackerCalibrationState() == ViolinRecordingPlugInEffect::TRACKER_CALIBRATED)
		{
			String msg = T("Loading calibration file will overwrite current calibration data. Continue?");
			if (!AlertWindow::showOkCancelBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), msg, T("Yes"), T("No")))
				return;
		}

//		WildcardFileFilter wildcardFilter(T("*.csv"), T("Comma Separated Value files"));
//		FileBrowserComponent browser(FileBrowserComponent::loadFileMode, File::nonexistent, &wildcardFilter, 0, false, false);
//		FileChooserDialogBox dialogBox(T("Open calibration file"), T("Please choose calibration file to open..."), browser, false, Colour::greyLevel(0.9f));

		FileChooser fileChooser(T("Open calibration file"), File::nonexistent, T("*.csv"), true);

//		if (dialogBox.show())
		if (fileChooser.browseForFileToOpen())
		{
			File selectedFile = fileChooser.getResult();//browser.getCurrentFile();
			if (!selectedFile.hasFileExtension(T(".csv")))
				selectedFile = selectedFile.withFileExtension(T(".csv"));
			if (!getFilter()->loadTrackerCalibrationFile(selectedFile.getFullPathName()))
			{
				String msg = T("Loading calibration file failed.");
				AlertWindow::showMessageBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), msg, T("Ok"));
			}

			updateEntireEditor(NULL); // update calibration section, update choosers
		}
	}
	else if (button == saveCalibrationButton_)
	{
//		WildcardFileFilter wildcardFilter(T("*.csv"), T("Comma Separated Value files"));
//		FileBrowserComponent browser(FileBrowserComponent::saveFileMode, File::nonexistent, &wildcardFilter, 0);
//		FileChooserDialogBox dialogBox(T("Save calibration file as..."), String::empty, browser, false, Colour::greyLevel(0.9f));
		// XXX: how to make above also give 'create folder' button? -> use FileChooser instead (??)

		FileChooser fileChooser(T("Save calibration file"), File::nonexistent, T("*.csv"), true);

//		if (dialogBox.show())
		if (fileChooser.browseForFileToSave(false))
		{
			File selectedFile = fileChooser.getResult();//browser.getCurrentFile();
			if (!selectedFile.hasFileExtension(T(".csv")))
				selectedFile = selectedFile.withFileExtension(T(".csv"));

			if (selectedFile.exists())
			{
				String msg = T("Overwrite existing file?");
				if (!AlertWindow::showOkCancelBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), msg, T("Yes"), T("No")))
					return;
			}

			getFilter()->saveTrackerCalibrationFile(selectedFile.getFullPathName());

			updateEntireEditor(NULL);
		}
	}
	else if (button == calibrateForceButton_)
	{
		if (getFilter()->getOperationMode() != ViolinRecordingPlugInEffect::CALIBRATING_FORCE)
		{
			AlertWindow dialog(T("Violin Recording Plug-In"), T("Which force calibration 3d points should be used?"), AlertWindow::WarningIcon);
			dialog.addButton("Take new", 0);
			dialog.addButton("From file", 1);
			if (getFilter()->areAllForceCalibrationStepsSet())
				dialog.addButton("Current", 2);
			int buttonClicked = dialog.runModalLoop();

			bool isSamplingForceCalibPoints = false;

			if (buttonClicked == 0)
			{
				// Start force calibration point sampling:
				getFilter()->startForceCalibTakePointsSeq();
				switchCalibrationSection(CALIBRATING);
				isSamplingForceCalibPoints = true;
			}
			else if (buttonClicked == 1)
			{
				FileChooser fileChooser(T("Open force calibration points file"), File::nonexistent, T("*.dat"), true);

				if (fileChooser.browseForFileToOpen())
				{
					File selectedFile = fileChooser.getResult();
					if (!selectedFile.hasFileExtension(T(".dat")))
						selectedFile = selectedFile.withFileExtension(T(".dat"));
					if (!getFilter()->loadForceCalibrationPointsFile(selectedFile.getFullPathName()))
					{
						String msg = T("Loading force calibration points file failed.");
						AlertWindow::showMessageBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), msg, T("Ok"));
					}
				}
			}
			else if (buttonClicked == 2)
			{
				// (do nothing)
			}

			if (!isSamplingForceCalibPoints)
			{
				if (!getFilter()->areAllForceCalibrationStepsSet())
				{
					AlertWindow::showMessageBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), T("Can't do force calibration recording until valid force calibration 3d points have been set."), T("Ok"));
				}
				else
				{
					// Start force calibration (recording):
					getFilter()->startForceCalibration();
				}
			}
		}
		else
		{
			// Stop force calibration (recording):
			getFilter()->endForceCalibration();
		}

		updateEntireEditor(NULL);
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	else if (button == calibrationTakeSampleButton_ && getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_TRACKER)
	{
		getFilter()->postTakeTrackerCalibrationSampleEvent();
	}
	else if (button == calibrationPrevStepButton_ && getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_TRACKER)
	{
		getFilter()->prevTrackerCalibrationStep();
		updateEntireEditor(NULL);
	}
	else if (button == calibrationNextStepButton_ && getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_TRACKER)
	{
		getFilter()->nextTrackerCalibrationStep();
		updateEntireEditor(NULL);
	}
	else if (button == calibrationStopButton_ && getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_TRACKER)
	{
		trackerCalibrationSequenceCanceled();
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	else if (button == calibrationTakeSampleButton_ && getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_FORCE_POINTS)
	{
		getFilter()->postTakeForceCalibrationSampleEvent();
	}
	else if (button == calibrationPrevStepButton_ && getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_FORCE_POINTS)
	{
		getFilter()->prevForceCalibrationStep();
		updateEntireEditor(NULL);
	}
	else if (button == calibrationNextStepButton_ && getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_FORCE_POINTS)
	{
		getFilter()->nextForceCalibrationStep();
		updateEntireEditor(NULL);
	}
	else if (button == calibrationStopButton_ && getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_FORCE_POINTS)
	{
		LOG_INFO_N("violin_recording_plugin", "Canceling force calibration point sampling...");

		getFilter()->endForceCalibTakePointsSeq();

		if (!getFilter()->areAllForceCalibrationStepsSet())
		{
			String msg = T("Warning: Not all required force calibration points were sampled!");
			AlertWindow::showMessageBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), msg, T("Ok"));
		}
		else
		{
			doSaveForceCalibrationDialog();

			// Start force calibration (recording):
			getFilter()->startForceCalibration();
		}

		switchCalibrationSection(NORMAL);
		updateEntireEditor(NULL);
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	//else if (button == calibrationTakeSampleButton_ && getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::SAMPLING_3D_MODEL)
	//{
	//	getFilter()->postTake3dModelSampleEvent();
	//}
	else if (button == calibrationPrevStepButton_ && getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::SAMPLING_3D_MODEL)
	{
		getFilter()->prevSampling3dModelStep();
		updateEntireEditor(NULL);
	}
	else if (button == calibrationNextStepButton_ && getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::SAMPLING_3D_MODEL)
	{
		getFilter()->nextSampling3dModelStep();
		updateEntireEditor(NULL);
	}
	else if (button == calibrationStopButton_ && getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::SAMPLING_3D_MODEL)
	{
		doSave3dModelDialog();

		getFilter()->endSampling3dModelSequence();
//		calibrationSequenceCanceled();
		switchCalibrationSection(NORMAL);
		updateEntireEditor(NULL);
	}
	// -----------------------------------------------------------------------------------
	else if (button == clearEventLogButton_)
	{
		getFilter()->getEventLog()->clear();
		updateEntireEditor(NULL);
	}
	// -----------------------------------------------------------------------------------
	else if (button == prevPhraseButton_)
	{
		getFilter()->decreaseCurPhraseIdx();
		updateEntireEditor(NULL);
	}
	else if (button == nextPhraseButton_)
	{
		getFilter()->increaseCurPhraseIdx();
		updateEntireEditor(NULL);
	}
	// -----------------------------------------------------------------------------------
	else if (button == makeModelButton_)
	{
		getFilter()->startSampling3dModelSequence();
		switchCalibrationSection(CALIBRATING);
		updateEntireEditor(NULL);
	}
	else if (button == loadModelButton_)
	{
		//if (getFilter()->getTrackerCalibrationState() == ViolinRecordingPlugInEffect::TRACKER_CALIBRATED)
		//{
		//	String msg = T("Loading calibration file will overwrite current calibration data. Continue?");
		//	if (!AlertWindow::showOkCancelBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), msg, T("Yes"), T("No")))
		//		return;
		//}

//		WildcardFileFilter wildcardFilter(T("*.dat"), T("Binary data files"));
//		FileBrowserComponent browser(FileBrowserComponent::loadFileMode, File::nonexistent, &wildcardFilter, 0, false, false);
//		FileChooserDialogBox dialogBox(T("Open violin 3d model file"), T("Please choose file to open..."), browser, false, Colour::greyLevel(0.9f));
		FileChooser fileChooser(T("Load 3D model file"), File::nonexistent, T("*.dat"), true);

		if (fileChooser.browseForFileToOpen())
//		if (dialogBox.show())
		{
			File selectedFile = fileChooser.getResult();//browser.getCurrentFile();
			if (!getFilter()->getViolinModel3d().loadFromFile(selectedFile.getFullPathName()))
			{
				String msg = T("Loading calibration file failed.");
				AlertWindow::showMessageBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), msg, T("Ok"));
			}
			else
			{
				if (scene3d_ != NULL)
					scene3d_->setViolinModel3d(getFilter()->getViolinModel3d());
			}

			updateEntireEditor(NULL); // update calibration section, update choosers
		}
	}
	else if (button == resetCameraButton_)
	{
		if (scene3d_ != NULL)
		{
			scene3d_->resetCamera();
		}
	}
	// -----------------------------------------------------------------------------------
	else if (button == manualSyncButton_)
	{
		getFilter()->setManualSyncEnabled(button->getToggleState());
		updateEntireEditor(NULL);
	}
	else if (button == plotSyncSigButton_)
	{
		getFilter()->setPlotSyncSigEnabled(button->getToggleState());
		updateEntireEditor(NULL);
	}
//t	else if (button == contRetriggerButton_)
//t	{
//t		getFilter()->setContRetriggerEnabled(button->getToggleState());
//t		updateEntireEditor(NULL);
//t	}
	else if (button == triggerSyncButton_)
	{
		getFilter()->triggerSyncSignal();
		updateEntireEditor(NULL); // manual sync trigger button
	}
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	else if (button == tuningRefToneButton_)
	{
		getFilter()->enableTuningRefTone(button->getToggleState());
	}
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEditor::comboBoxChanged(ComboBox *comboBoxThatHasChanged)
{
	int id = comboBoxThatHasChanged->getSelectedId();
	if (id > 0)
	{
		getFilter()->setComPortIdx(id - 1);
	}
}

// ---------------------------------------------------------------------------------------

int ViolinRecordingPlugInEditor::getNumRows()
{
	return getFilter()->getEventLog()->getSize();
}

void ViolinRecordingPlugInEditor::paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected)
{
	g.setColour(getFilter()->getEventLog()->getItemColour(rowNumber));
	g.drawText(getFilter()->getEventLog()->getItem(rowNumber), 4, 0, width - 4, height, Justification::centredLeft, true);
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEditor::textEditorReturnKeyPressed(TextEditor &editor)
{
	if (getFilter()->setScoreListItem(editor.getText())) // only sets item if name is valid, also updates take idx
		getFilter()->saveUpdatedScoreList();
	updatePhraseSection();
}

void ViolinRecordingPlugInEditor::textEditorEscapeKeyPressed(TextEditor &editor)
{
	// Revert to previous item name (update without setting item):
	updatePhraseSection();
}

void ViolinRecordingPlugInEditor::textEditorFocusLost(TextEditor &editor)
{
	if (getFilter()->setScoreListItem(editor.getText())) // only sets item if name is valid, also updates take idx
		getFilter()->saveUpdatedScoreList();
	updatePhraseSection();
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEditor::sliderValueChanged(Slider *slider)
{
	if (slider == trackerToAudioSyncOffsetSlider_)
	{
		getFilter()->setTrackerToAudioSyncOffset((int)slider->getValue());
	}
	else if (slider == arduinoToTrackerSyncOffsetSlider_)
	{
		getFilter()->setArduinoToTrackerSyncOffset((int)slider->getValue());
	}
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEditor::updateEntireEditor(void *)
{
	updateEditorSize();
	updateConfigurationSection();
	updateTrackerConnectSection();
	updateCalibrationSection();
	updateChoosers();
	updatePhraseSection();
	updateLoggerListBoxAsynchronous(NULL);
	updateScene3dSection();
	updateSyncSection();
}

void ViolinRecordingPlugInEditor::updateLoggerListBoxAsynchronous(void *)
{
	trackerLogListBox_->updateContent();

	//if (getNumRows() > 0)
	//	trackerLogListBox_->selectRow(getNumRows()-1, false, true);

	// hack to make listbox update properly
	int b = getNumRows() - 12;
	if (b < 0)
		b = 0;
	for (int i = b; i < getNumRows(); ++i)
	{
		trackerLogListBox_->selectRow(i, false, true);
	}

	if (getNumRows() == 0)
	{
		clearEventLogButton_->setEnabled(false);
	}
	else
	{
		clearEventLogButton_->setEnabled(true);
	}
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEditor::paint(Graphics& g)
{
	g.fillAll(Colour::greyLevel(0.9f));
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEditor::updateEditorSize()
{
	int w;
	int h;

	w = 125*gridUnitPx;
	h = (120 + 10)*gridUnitPx;

	if (getFilter()->isUseScene3dEnabled())
	{
		w += 108*gridUnitPx;
	}

	if (getWidth() != w || getHeight() != h)
	{
		LOG_INFO_N("violin_recording_plugin", "Resizing editor...");
		setSize(w, h);
	}
}

void ViolinRecordingPlugInEditor::updateConfigurationSection()
{
	// Use gages toggle:
	useGagesButton_->setToggleState(getFilter()->isUseGagesEnabled(), false);
	if (!isEffectProcessingSuspended_ &&
		getFilter()->getRecordingState() == ViolinRecordingPlugInEffect::NOT_RECORDING && 
		(getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::NORMAL || getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_FORCE) && 
		getFilter()->getTrackerState() == ViolinRecordingPlugInEffect::TRACKER_DISCONNECTED)
	{
		if (ComPort::getNumComPorts() > 0)
			useGagesButton_->setEnabled(true);
		else
			useGagesButton_->setEnabled(false);
	}
	else
	{
		useGagesButton_->setEnabled(false);
	}

	// Com port combo box:
	if (comPortComboBox_->getNumItems() > 0)
	{
		int comPortIdx = getFilter()->getGagesComPort();
		if (comPortIdx < 0 || comPortIdx >= comPortComboBox_->getNumItems())
			comPortIdx = 0;
		comPortComboBox_->setSelectedId(1 + getFilter()->getGagesComPort(), false);
	}
	if (!isEffectProcessingSuspended_ &&
		getFilter()->getRecordingState() == ViolinRecordingPlugInEffect::NOT_RECORDING && 
		(getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::NORMAL || getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_FORCE) && 
		getFilter()->getTrackerState() == ViolinRecordingPlugInEffect::TRACKER_DISCONNECTED)
	{
		if (useGagesButton_->getToggleState())
			comPortComboBox_->setEnabled(true);
		else
			comPortComboBox_->setEnabled(false);
	}
	else
	{
		comPortComboBox_->setEnabled(false);
	}

	// Use stereo toggle:
	useStereoButton_->setToggleState(getFilter()->isUseStereoEnabled(), false);
	if (!isEffectProcessingSuspended_ &&
		getFilter()->getRecordingState() == ViolinRecordingPlugInEffect::NOT_RECORDING && 
		(getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::NORMAL || getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_FORCE) && 
		getFilter()->getTrackerState() == ViolinRecordingPlugInEffect::TRACKER_DISCONNECTED &&
		getFilter()->getNumInputChannels() >= 2)
	{
		useStereoButton_->setEnabled(true);
	}
	else
	{
		useStereoButton_->setEnabled(false);
	}

	// Use auto string estimation toggle:
	useAutoStringButton_->setToggleState(getFilter()->isAutoStringEnabled(), false);
	if (!isEffectProcessingSuspended_ &&
		getFilter()->getRecordingState() == ViolinRecordingPlugInEffect::NOT_RECORDING && 
		(getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::NORMAL || getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_FORCE) && 
		getFilter()->getTrackerState() == ViolinRecordingPlugInEffect::TRACKER_DISCONNECTED)
	{
		useAutoStringButton_->setEnabled(true);
	}
	else
	{
		useAutoStringButton_->setEnabled(false);
	}

	// Use 3d scene toggle:
	useScene3dButton_->setToggleState(getFilter()->isUseScene3dEnabled(), false);
	if (!isEffectProcessingSuspended_ &&
		getFilter()->getRecordingState() == ViolinRecordingPlugInEffect::NOT_RECORDING && 
		(getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::NORMAL || getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_FORCE) && 
		getFilter()->getTrackerState() == ViolinRecordingPlugInEffect::TRACKER_DISCONNECTED)
	{
		useScene3dButton_->setEnabled(true);
	}
	else
	{
		useScene3dButton_->setEnabled(false);
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// Bow force label:
	int wantUseGages = (int)getFilter()->isUseGagesEnabled();
	if (isUsingGages_ == -1 || isUsingGages_ != wantUseGages) // to avoid redundant calls to setYLim() as it clears the plot
	{
		if (wantUseGages)
		{
#if (ENABLE_OPTICAL_SENSOR != 0)
			bowForceLabel_->setText(T("Optical, gages, load cell (all normalized):"), false);
			bowForcePlot_->setYLim(0.0f, 100.0);
#else
			bowForceLabel_->setText(T("PBF (-1 .. +2 cm) & Bow Force (normalized):"), false);
			bowForcePlot_->setYLim(0.0f, 100.0f);
#endif
		}
		else
		{
			bowForceLabel_->setText(T("Pseudo Bow Force (-1 .. +2 cm):"), false);
			bowForcePlot_->setYLim(0.0, 100.0f);
		}

		isUsingGages_ = wantUseGages;
	}

	// Sensor acceleration / sync signals label:
	if (!getFilter()->isPlotSyncSigEnabled())
	{
#if (PLOT_STICK_BRIDGE_DISTANCE == 0)
		summedSensorAccelerationLabel_->setText(T("Summed Sensor Accelerations (noise):"), false);
#else
		summedSensorAccelerationLabel_->setText(T("Stick-bridge distance (0-10 cm):"), false);
#endif
		noiseAndSyncPlot_->setNumDisplayEnabled(true);
	}
	else
	{
		summedSensorAccelerationLabel_->setText(T("Sync signals:"), false);
		noiseAndSyncPlot_->setNumDisplayEnabled(false);
	}
}

void ViolinRecordingPlugInEditor::updateTrackerConnectSection()
{
	// Connect/disconnect buttons:
	if (!isEffectProcessingSuspended_ &&
		getFilter()->getRecordingState() == ViolinRecordingPlugInEffect::NOT_RECORDING && 
		(getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::NORMAL || getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_FORCE))
	{
		if (getFilter()->getTrackerState() == ViolinRecordingPlugInEffect::TRACKER_CONNECTED)
		{
			// Tracker is connected:
			connectTrackerButton_->setEnabled(false);
			disconnectTrackerButton_->setEnabled(true);
		}
		else if (getFilter()->getTrackerState() == ViolinRecordingPlugInEffect::TRACKER_DISCONNECTED || 
			getFilter()->getTrackerState() == ViolinRecordingPlugInEffect::TRACKER_FAILED)
		{
			// Tracker is disconnected (initial state or due to error):
			connectTrackerButton_->setEnabled(true);
			disconnectTrackerButton_->setEnabled(false);
		}
		else if (getFilter()->isTrackerBusy())
		{
			// Tracker is busy connecting/disconnecting:
			connectTrackerButton_->setEnabled(false);
			disconnectTrackerButton_->setEnabled(false);
		}
	}
	else
	{
		// Recording or not processing, disable buttons:
		connectTrackerButton_->setEnabled(false);
		disconnectTrackerButton_->setEnabled(false);
	}

	// Tracker status label:
	String statusLabel = T("Tracker status: ");
	if (getFilter()->getTrackerState() == ViolinRecordingPlugInEffect::TRACKER_DISCONNECTED)
	{
		statusLabel += T("Not connected.");
	}
	else if (getFilter()->getTrackerState() == ViolinRecordingPlugInEffect::TRACKER_CONNECTED)
	{
		statusLabel += T("Connected.");
	}
	else if (getFilter()->getTrackerState() == ViolinRecordingPlugInEffect::TRACKER_FAILED)
	{
		statusLabel += T("Not connected (error).");
	}
	else if (getFilter()->isTrackerBusy())
	{
		statusLabel += T("Busy.");
	}
	else
	{
		statusLabel += T("Unknown (error).");
	}

	trackerStatusLabel_->setText(statusLabel, false); // don't emit change event
}

void ViolinRecordingPlugInEditor::updateCalibrationSection()
{
	bool ableToUseProcessing = (isEffectProcessingSuspended_ == false);
	bool ableToSaveCalibration = getFilter()->hasTrackerCalibrationBeenModified();

	bool ableToCalibrateForce = (ableToUseProcessing &&
		getFilter()->getScoreListFilename() != String::empty && 
		getFilter()->getOutputPath() != String::empty); // score list and output path must be set before you can calibrate force (take points + make recording)

	if (!isEffectProcessingSuspended_ &&
		getFilter()->getRecordingState() == ViolinRecordingPlugInEffect::NOT_RECORDING &&
		(getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::NORMAL || getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_FORCE))
	{
		if (getFilter()->getTrackerState() == ViolinRecordingPlugInEffect::TRACKER_CONNECTED)
		{
			if (getFilter()->getTrackerCalibrationState() == ViolinRecordingPlugInEffect::TRACKER_CALIBRATED)
			{
				// Already calibrated, enable all buttons:
				calibrateButton_->setEnabled(ableToUseProcessing);
				calibrateForceButton_->setEnabled(ableToCalibrateForce);
				loadCalibrationButton_->setEnabled(true);
				saveCalibrationButton_->setEnabled(ableToSaveCalibration);
			}
			else
			{
				// Not yet calibrated, only enable buttons that calibrate:
				calibrateButton_->setEnabled(ableToUseProcessing);
				calibrateForceButton_->setEnabled(false); // don't allow calibrating force until tracker is calibrated
				loadCalibrationButton_->setEnabled(true);
				saveCalibrationButton_->setEnabled(false);
			}
		}
		else
		{
			// Not yet connected or busy, disable buttons (except save/load):
			calibrateButton_->setEnabled(false);
			calibrateForceButton_->setEnabled(false);
			if (getFilter()->isTrackerBusy())
			{
				loadCalibrationButton_->setEnabled(false);
				saveCalibrationButton_->setEnabled(false);
			}
			else
			{
				loadCalibrationButton_->setEnabled(true);
				saveCalibrationButton_->setEnabled(ableToSaveCalibration);
			}
		}
	}
	else
	{
		// Recording or not processing, disable buttons:
		calibrateButton_->setEnabled(false);
		calibrateForceButton_->setEnabled(false);
		loadCalibrationButton_->setEnabled(false);
		saveCalibrationButton_->setEnabled(false);
	}

	// Force calibration button:
	if (getFilter()->getOperationMode() != ViolinRecordingPlugInEffect::CALIBRATING_FORCE)
		calibrateForceButton_->setButtonText("Start calib. force");
	else
		calibrateForceButton_->setButtonText("Stop calib. force");

	// Calibration status label:
	String statusLabel = T("Calibrated: ");
	if (getFilter()->getTrackerCalibrationState() == ViolinRecordingPlugInEffect::TRACKER_CALIBRATED)
	{
		if (getFilter()->hasTrackerCalibrationBeenModified())
			statusLabel += T("Yes (modified).");
		else
			statusLabel += T("Yes.");
	}
	else if (getFilter()->getTrackerCalibrationState() == ViolinRecordingPlugInEffect::TRACKER_NOT_CALIBRATED)
	{
		statusLabel += T("No.");
	}
	else
	{
		statusLabel += T("Unknown (error).");
	}

	calibrationStatusLabel_->setText(statusLabel, false); // don't emit change event

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	if (getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_TRACKER)
	{
		if (getFilter()->getTrackerCalibrationStep() == 0)
			calibrationPrevStepButton_->setEnabled(false);
		else
			calibrationPrevStepButton_->setEnabled(true);

		if (getFilter()->getTrackerCalibrationStep() >= TrackerCalibration::NUM_STEPS - 1)
			calibrationNextStepButton_->setEnabled(false);
		else
			calibrationNextStepButton_->setEnabled(true);
	}
	else if (getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_FORCE_POINTS)
	{
		if (getFilter()->getForceCalibrationStep() == 0)
			calibrationPrevStepButton_->setEnabled(false);
		else
			calibrationPrevStepButton_->setEnabled(true);

		if (getFilter()->getForceCalibrationStep() >= ForceCalibration::NUM_STEPS - 1)
			calibrationNextStepButton_->setEnabled(false);
		else
			calibrationNextStepButton_->setEnabled(true);
	}
	else if (getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::SAMPLING_3D_MODEL)
	{
		calibrationTakeSampleButton_->setEnabled(false); // only allow taking samples using stylus button

		if (getFilter()->getViolinModel3d().getPathStep() == 0)
			calibrationPrevStepButton_->setEnabled(false);
		else
			calibrationPrevStepButton_->setEnabled(true);

		if (getFilter()->getViolinModel3d().getPathStep() >= getFilter()->getViolinModel3d().getNumPathSteps() - 1)
			calibrationNextStepButton_->setEnabled(false);
		else
			calibrationNextStepButton_->setEnabled(true);
	}
}

void ViolinRecordingPlugInEditor::updateChoosers()
{
	// Set text:
	if (getFilter()->getScoreListFilename() != String::empty)
		scoreListFileChooser_->setCurrentFile(File(getFilter()->getScoreListFilename()), true, false); // add to recent list, don't emit change signal
	
	if (getFilter()->getOutputPath() != String::empty)
		outputPathChooser_->setCurrentFile(File(getFilter()->getOutputPath()), true, false); // add to recent list, don't emit change signal

	// Enable/disable choosers:
	if (!isEffectProcessingSuspended_ &&
		getFilter()->getRecordingState() == ViolinRecordingPlugInEffect::NOT_RECORDING && 
		(getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::NORMAL || getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_FORCE))
	{
		if (getFilter()->getTrackerState() == ViolinRecordingPlugInEffect::TRACKER_CONNECTED && 
			getFilter()->getTrackerCalibrationState() == ViolinRecordingPlugInEffect::TRACKER_CALIBRATED)
		{
			// Connected and calibrated, enable:
			scoreListFileChooser_->setEnabled(true);
			outputPathChooser_->setEnabled(true);
		}
		else
		{
			// Not yet connected and calibrated, disable:
			scoreListFileChooser_->setEnabled(false);
			outputPathChooser_->setEnabled(false);
		}
	}
	else
	{
		// Recording or not processing, disable choosers:
		scoreListFileChooser_->setEnabled(false);
		outputPathChooser_->setEnabled(false);
	}
}

void ViolinRecordingPlugInEditor::updatePhraseSection()
{
	if (getFilter()->getTrackerState() == ViolinRecordingPlugInEffect::TRACKER_CONNECTED &&
		getFilter()->getTrackerCalibrationState() == ViolinRecordingPlugInEffect::TRACKER_CALIBRATED && 
		getFilter()->getScoreListFilename() != String::empty && 
		getFilter()->getOutputPath() != String::empty)
	{
		bool prevPhraseEnable;
		bool nextPhraseEnable;

		if (!isEffectProcessingSuspended_ &&
			getFilter()->getRecordingState() == ViolinRecordingPlugInEffect::NOT_RECORDING &&
			(getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::NORMAL || getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_FORCE))
		{
			phraseLabel_->setText(T("Next phrase:"), false);
			phraseLabel_->setColour(Label::textColourId, Colours::black);

			phraseIndexLabel_->setColour(Label::textColourId, Colours::black);

			phraseNameEditor_->setColour(TextEditor::textColourId, Colours::black);
			phraseNameEditor_->applyFontToAllText(phraseNameEditor_->getFont()); // call this to enforce colour change for existing text
			phraseNameEditor_->setEnabled(true);

			if (getFilter()->getCurPhraseTakeIdx() == 0)
				takeIndexLabel_->setColour(Label::textColourId, Colours::black);
			else
				takeIndexLabel_->setColour(Label::textColourId, Colours::red);

			prevPhraseEnable = true;
			nextPhraseEnable = true;
		}
		else
		{
			phraseLabel_->setText(T("Recording phrase:"), false);
			phraseLabel_->setColour(Label::textColourId, Colours::red);

			phraseIndexLabel_->setColour(Label::textColourId, Colours::red);

			phraseNameEditor_->setColour(TextEditor::textColourId, Colours::red);
			phraseNameEditor_->applyFontToAllText(phraseNameEditor_->getFont()); // call this to enforce colour change for existing text
			phraseNameEditor_->setEnabled(false);

			takeIndexLabel_->setColour(Label::textColourId, Colours::red);

			prevPhraseEnable = false;
			nextPhraseEnable = false;
		}

		phraseIndexLabel_->setText(String::formatted(T("%03d"), getFilter()->getCurPhraseIdx() + 1), false);

		phraseNameEditor_->setText(getFilter()->getCurPhraseName(), false);

		String takeIdxStr = String(T("take ")) + String::formatted(T("%03d"), getFilter()->getCurPhraseTakeIdx() + 1);
		takeIndexLabel_->setText(takeIdxStr, false);

		if (getFilter()->getCurPhraseIdx() > 0)
			prevPhraseButton_->setEnabled(prevPhraseEnable);
		else
			prevPhraseButton_->setEnabled(false);

		if (getFilter()->getCurPhraseIdx() < getFilter()->getNumPhrases() - 1)
			nextPhraseButton_->setEnabled(nextPhraseEnable);
		else
			nextPhraseButton_->setEnabled(false);
	}
	else
	{
		phraseLabel_->setText(T("Next phrase:"), false);
		phraseLabel_->setColour(Label::textColourId, Colours::black);
		phraseIndexLabel_->setText(T("---"), false);
		phraseIndexLabel_->setColour(Label::textColourId, Colours::black);
		phraseNameEditor_->setText(T(""), false);
		phraseNameEditor_->setEnabled(false);
		phraseNameEditor_->setColour(TextEditor::textColourId, Colours::black);
		takeIndexLabel_->setText(T("take ---"), false);
		prevPhraseButton_->setEnabled(false);
		nextPhraseButton_->setEnabled(false);
	}
}

void ViolinRecordingPlugInEditor::updateScene3dSection()
{
	if (getFilter()->isUseScene3dEnabled())
	{
		if (scene3d_ == NULL)
		{
			LOG_INFO_N("violin_recording_plugin", "Creating 3d scene...");
			scene3d_ = new Scene3d(T("open gl 3d scene"));
			if (getFilter()->hasValidLastCamera())
				scene3d_->setCamera(getFilter()->getLastCamera());
//			LOG_DEBUG_N("violin_recording_plugin", "DEBUG addAndMakeVisible()");
			addAndMakeVisible(scene3d_);
//			LOG_DEBUG_N("violin_recording_plugin", "DEBUG setBounds()");
			scene3d_->setBounds(125*gridUnitPx, (2 + 4)*gridUnitPx, 105*gridUnitPx, 105*gridUnitPx);
//			LOG_DEBUG_N("violin_recording_plugin", "DEBUG atomic pointer assignement");
			isScene3dValid_.set(true);
			// .. timer started on connect ..
		}
	}
	else
	{
		if (scene3d_ != NULL)
		{
			LOG_INFO_N("violin_recording_plugin", "Destroying 3d scene...");

			isScene3dValid_.set(false);

			// store last camera coordinates:
			getFilter()->setLastKnownCamera(scene3d_->getCamera());

			// (no need for thread synchronization, this is only done when not in-use by effect)
			// timer will normally be stopped already (otherwise done in destructor)
			removeChildComponent(scene3d_); // also deletes scene3d_
			scene3d_ = NULL;
		}
	}

	if (!isEffectProcessingSuspended_ &&
		getFilter()->getRecordingState() == ViolinRecordingPlugInEffect::NOT_RECORDING &&
		(getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::NORMAL || getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_FORCE) && 
		getFilter()->getTrackerState() == ViolinRecordingPlugInEffect::TRACKER_CONNECTED)

	{
		makeModelButton_->setEnabled(true);
//		loadModelButton_->setEnabled(true);
	}
	else
	{
		makeModelButton_->setEnabled(false);
//		loadModelButton_->setEnabled(false);
	}

	if (getFilter()->getRecordingState() == ViolinRecordingPlugInEffect::NOT_RECORDING &&
		(getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::NORMAL || getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_FORCE) && 
		getFilter()->getTrackerState() == ViolinRecordingPlugInEffect::TRACKER_DISCONNECTED)
	{
		loadModelButton_->setEnabled(true);
	}
	else
	{
		loadModelButton_->setEnabled(false);
	}
}

void ViolinRecordingPlugInEditor::updateSyncSection()
{
	// Set state of widgets:
	manualSyncButton_->setToggleState(getFilter()->isManualSyncEnabled(), false);
	trackerToAudioSyncOffsetSlider_->setValue((double)getFilter()->getTrackerToAudioSyncOffset(), false, false);
	arduinoToTrackerSyncOffsetSlider_->setValue((double)getFilter()->getArduinoToTrackerSyncOffset(), false, false);
	plotSyncSigButton_->setToggleState(getFilter()->isPlotSyncSigEnabled(), false);
//t	contRetriggerButton_->setToggleState(getFilter()->isContRetriggerEnabled(), false);

	// Enable/disable widgets:
#if (ALLOW_SYNC_CHANGES_WHILE_RECORDING != 0)
	if (isEffectProcessingSuspended_ || getFilter()->isTrackerBusy())
#else
	if (isEffectProcessingSuspended_ || getFilter()->isTrackerBusy() || getFilter()->getRecordingState() == ViolinRecordingPlugInEffect::RECORDING)
#endif
	{
		// processing suspended (disable all)
		manualSyncButton_->setEnabled(false);
		trackerToAudioSyncOffsetSlider_->setEnabled(false);
		arduinoToTrackerSyncOffsetSlider_->setEnabled(false);
		plotSyncSigButton_->setEnabled(false);
//t		contRetriggerButton_->setEnabled(false);
		triggerSyncButton_->setEnabled(false);
	}
	else
	{
		if (getFilter()->getTrackerState() == ViolinRecordingPlugInEffect::TRACKER_CONNECTED)
		{
			// after connecting tracker

			manualSyncButton_->setEnabled(true);
//			plotSyncSigButton_->setEnabled(false); // can only be changed while disconnected
			if (!getFilter()->isManualSyncEnabled())
			{
				plotSyncSigButton_->setEnabled(true);
//t				contRetriggerButton_->setEnabled(true); // automatic sync
			}
			else
			{
				plotSyncSigButton_->setEnabled(false);
//t				contRetriggerButton_->setEnabled(false); // manual sync
			}

			if (!getFilter()->isManualSyncEnabled())
			{
				// automatic sync
				trackerToAudioSyncOffsetSlider_->setEnabled(false);
				arduinoToTrackerSyncOffsetSlider_->setEnabled(false);

				if (getFilter()->isContRetriggerEnabled())
					triggerSyncButton_->setEnabled(false);
				else
					triggerSyncButton_->setEnabled(true);
			}
			else
			{
				// manual sync
				trackerToAudioSyncOffsetSlider_->setEnabled(true);
				if (getFilter()->isUseGagesEnabled())
					arduinoToTrackerSyncOffsetSlider_->setEnabled(true);
				else
					arduinoToTrackerSyncOffsetSlider_->setEnabled(false);

				triggerSyncButton_->setEnabled(false);
			}
		}
		else // (TRACKER_DISCONNECTED || TRACKER_FAILED) 
		{
			// before connecting tracker

			manualSyncButton_->setEnabled(true);

			if (!getFilter()->isManualSyncEnabled())
			{
				// automatic sync
				plotSyncSigButton_->setEnabled(true);
//t				contRetriggerButton_->setEnabled(true);
			}
			else
			{
				// manual sync
				plotSyncSigButton_->setEnabled(false);
//t				contRetriggerButton_->setEnabled(false);
			}

			trackerToAudioSyncOffsetSlider_->setEnabled(false);
			arduinoToTrackerSyncOffsetSlider_->setEnabled(false);
			triggerSyncButton_->setEnabled(false);
		}
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	if (!getFilter()->isManualSyncEnabled() && getFilter()->doesSyncRequireRetrigger())
		triggerSyncButton_->setColour(TextButton::buttonColourId, Colour(255, 0, 0));
	else
		triggerSyncButton_->setColour(TextButton::buttonColourId, LookAndFeel::getDefaultLookAndFeel().findColour(TextButton::buttonColourId));

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// State:
	tuningRefToneButton_->setToggleState(getFilter()->getUseTuningRefTone(), false);

	// Enable/disable:
	if (isEffectProcessingSuspended_ || getFilter()->isTrackerBusy() || getFilter()->getRecordingState() == ViolinRecordingPlugInEffect::RECORDING)
	{
		tuningRefToneButton_->setEnabled(false);
	}
	else
	{
		tuningRefToneButton_->setEnabled(true);
	}
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEditor::timerCallback()
{
	// Check if score list was changed by external application and optionally reload:
	handleExternalScoreListUpdates();

#if (ENABLE_CHECK_PROCESSING_DEAD != 0)
	// Check if effect processing is active:
	if (lastProcessingDeadFlagSetTime_ != 0)
	{
		int64 timeSinceDeadFlagSet = Time::getMillisecondCounter() - lastProcessingDeadFlagSetTime_;

		// Wait for next timer call if checks are too close:
		if (timeSinceDeadFlagSet < 1000) // normally should be 1500, but just in case two timerCallback() calls happen very close in time
		{
			LOG_INFO_N("violin_recording_plugin", "Audio processing alive checks too close (ignore).");
			return; // skip resetting alive/dead flag
		}
	
		// Check flag:
		bool processingStillDead = getFilter()->isProcessingDead();

		// Processing dead->alive:
		if (processingStillDead && !isEffectProcessingSuspended_)
		{
			LOG_INFO_N("violin_recording_plugin", "Audio processing is dead or blocking (disconnecting tracker if needed).");
			LOG_INFO_N("violin_recording_plugin", concat::formatStr("Time: %s.", concat::getSystemTime().c_str()));
			LOG_INFO_N("violin_recording_plugin", concat::formatStr("Time since setting dead flag: %d.", (int)timeSinceDeadFlagSet));
			LOG_INFO_N("violin_recording_plugin", concat::formatStr("Processing debug locator: %d.", (int)getFilter()->processingDebugLocator));
			getFilter()->disconnectTrackerAsynch();
			isEffectProcessingSuspended_ = true;
			updateEntireEditor(NULL);
		}
		// Processing alive->dead:
		else if (!processingStillDead && isEffectProcessingSuspended_)
		{
			LOG_INFO_N("violin_recording_plugin", "Audio processing is alive.");
			isEffectProcessingSuspended_ = false;
			updateEntireEditor(NULL);
		}
	}

	// Set flag and remember time:
	getFilter()->setProcessingDeadFlag();
	lastProcessingDeadFlagSetTime_ = Time::getMillisecondCounter();
#endif
}

void ViolinRecordingPlugInEditor::handleExternalScoreListUpdates()
{
	String curScoreListFilename;
	// LOCK
	{
		const ScopedLock scopedScoreListLock(scoreListLock_);
		curScoreListFilename = getFilter()->getScoreListFilename();
	}
	// UNLOCK

	if (curScoreListFilename != String::empty)
	{
		File scoreList(curScoreListFilename);
		Time scoreListModTime;
		// LOCK
		{
			const ScopedLock scopedScoreListLock(scoreListLock_);
			scoreListModTime = scoreList.getLastModificationTime();
		}
		// UNLOCK
		if (scoreListModTime != lastCheckedScoreListModTime_)
		{
			bool changeValid = (lastCheckedScoreListModTime_ != Time(1970, 1, 1, 1, 1));
			lastCheckedScoreListModTime_ = scoreListModTime;
			if (changeValid)
			{
				String msg = T("Score list was updated externally. Reload?");
				if (AlertWindow::showOkCancelBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), msg, T("Yes"), T("No")))
				{
					// LOCK
					{
						const ScopedLock scopedScoreListLock(scoreListLock_);
						int curPhraseIdx = getFilter()->getCurPhraseIdx();
						String curPhraseName = getFilter()->getCurPhraseName();
						bool ok = getFilter()->setScoreListFilenameAndLoadIfValid(getFilter()->getScoreListFilename());
						if (ok)
						{
							getFilter()->trySetCurPhraseIdxFromNameElseIdx(curPhraseName, curPhraseIdx);
						}
					}
					// UNLOCK

					updateEntireEditor(NULL);
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEditor::switchCalibrationSection(CalibrationSectionMode mode)
{
	if (mode == NORMAL)
	{
		calibrateButton_->setVisible(true);
		loadCalibrationButton_->setVisible(true);
		saveCalibrationButton_->setVisible(true);
		calibrateForceButton_->setVisible(true);
	}
	else
	{
		calibrateButton_->setVisible(false);
		loadCalibrationButton_->setVisible(false);
		saveCalibrationButton_->setVisible(false);
		calibrateForceButton_->setVisible(false);
	}

	if (mode == CALIBRATING)
	{
		calibrationTakeSampleButton_->setVisible(true);
		calibrationPrevStepButton_->setVisible(true);
		calibrationNextStepButton_->setVisible(true);
		calibrationStopButton_->setVisible(true);
	}
	else
	{
		calibrationTakeSampleButton_->setVisible(false);
		calibrationPrevStepButton_->setVisible(false);
		calibrationNextStepButton_->setVisible(false);
		calibrationStopButton_->setVisible(false);
	}
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEditor::startTrackerCalibration()
{
	getFilter()->startTrackerCalibrationSequence();

//g	bowDisplacementPlot_->clear();
//g	bowForcePlot_->clear();
//g	bowBridgeDistancePlot_->clear();
//g	summedSensorAccelerationsPlot_->clear();

	switchCalibrationSection(CALIBRATING);

	updateEntireEditor(NULL);
}

void ViolinRecordingPlugInEditor::endTrackerCalibrationSequenceReached(void *)
{
	if (!getFilter()->areAllTrackerCalibrationStepsSet())
	{
		String msg = T("Warning: Not all steps of the calibration were set!");
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), msg, T("Ok"));
	}
	
	LOG_INFO_N("violin_recording_plugin", "Done calibrating!");

//g	summedSensorAccelerationsPlot_->clear(); // so all plots run synchronously

	switchCalibrationSection(NORMAL);

	updateEntireEditor(NULL);
}

void ViolinRecordingPlugInEditor::trackerCalibrationSequenceCanceled()
{
	if (!getFilter()->areAllTrackerCalibrationStepsSet())
	{
		String msg = T("Warning: Not all steps of the calibration were set!");
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), msg, T("Ok"));
	}

	LOG_INFO_N("violin_recording_plugin", "Calibration canceled!");

	getFilter()->endTrackerCalibrationSequence(false);

	switchCalibrationSection(NORMAL);

	updateEntireEditor(NULL);
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEditor::endSampling3dModelSequenceReached(void *)
{
	if (!getFilter()->getViolinModel3d().areAllStepsSet())
	{
		String msg = T("Warning: Not all points of model were sampled!");
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), msg, T("Ok"));
	}
	else
	{
		doSave3dModelDialog();

		scene3d_->setViolinModel3d(getFilter()->getViolinModel3d());
	}

	LOG_INFO_N("violin_recording_plugin", "Done sampling 3d model!");

	switchCalibrationSection(NORMAL);

	updateEntireEditor(NULL);
}

void ViolinRecordingPlugInEditor::doSave3dModelDialog()
{
	// XXX: note in msg if all steps were set or not!
	String msg = T("Save 3d model to file?");
	if (AlertWindow::showOkCancelBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), msg, T("Yes"), T("No")))
	{
		FileChooser fileChooser(T("Save 3D model file"), File::nonexistent, T("*.dat"), true);

		if (fileChooser.browseForFileToSave(false))
		{
			File selectedFile = fileChooser.getResult();
			if (!selectedFile.hasFileExtension(T(".dat")))
				selectedFile = selectedFile.withFileExtension(T(".dat"));

			if (selectedFile.exists())
			{
				String msg = T("Overwrite existing file?");
				if (AlertWindow::showOkCancelBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), msg, T("Yes"), T("No")))
				{
					getFilter()->getViolinModel3d().saveToFile(selectedFile.getFullPathName());
				}
			}
			else
			{
				getFilter()->getViolinModel3d().saveToFile(selectedFile.getFullPathName());
			}
		}
	}
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEditor::startForceCalibration()
{
	switchCalibrationSection(NORMAL);

	updateEntireEditor(NULL);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ViolinRecordingPlugInEditor::endForceCalibrationSequenceReached(void *)
{
	LOG_INFO_N("violin_recording_plugin", "Done sampling points for force calibration!");

	if (!getFilter()->areAllForceCalibrationStepsSet())
	{
		String msg = T("Warning: Not all required force calibration points were sampled!");
		AlertWindow::showMessageBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), msg, T("Ok"));
	}
	else
	{
		doSaveForceCalibrationDialog();

		// Start force calibration (recording):
		getFilter()->startForceCalibration();
	}

	switchCalibrationSection(NORMAL);
	updateEntireEditor(NULL);
}

void ViolinRecordingPlugInEditor::doSaveForceCalibrationDialog()
{
	// XXX: note in msg if all steps were set or not!
	String msg = T("Save force calibration points to file?");
	if (AlertWindow::showOkCancelBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), msg, T("Yes"), T("No")))
	{
		FileChooser fileChooser(T("Save force calibration points file"), File::nonexistent, T("*.dat"), true);

		if (fileChooser.browseForFileToSave(false))
		{
			File selectedFile = fileChooser.getResult();
			if (!selectedFile.hasFileExtension(T(".dat")))
				selectedFile = selectedFile.withFileExtension(T(".dat"));

			if (selectedFile.exists())
			{
				String msg = T("Overwrite existing file?");
				if (AlertWindow::showOkCancelBox(AlertWindow::WarningIcon, T("Violin Recording Plug-In"), msg, T("Yes"), T("No")))
				{
					getFilter()->saveForceCalibrationFile(selectedFile.getFullPathName());
				}	
			}
			else
			{
				getFilter()->saveForceCalibrationFile(selectedFile.getFullPathName());
			}
		}
	}
}

// ---------------------------------------------------------------------------------------

void ViolinRecordingPlugInEditor::doEndRecordingPhraseDialog(void *)
{
	// Ask user to go to next phrase or repeat current:
	if (getFilter()->getCurPhraseIdx() < getFilter()->getNumPhrases() - 1)
	{
		String msg = T("Go to next phrase or repeat current?");
		if (getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_FORCE)
			msg += T(" Going to next phrase will end force calibration.");

		if (AlertWindow::showOkCancelBox(AlertWindow::QuestionIcon, T("Violin Recording Plug-In"), msg, T("Next"), T("Repeat")))
		{
			// Next:
			bool couldIncrTakeIdx = getFilter()->incrCurPhraseTakeIdx(); // increment take index of current (just recorded) phrase

			if (!couldIncrTakeIdx)
				AlertWindow::showMessageBox(AlertWindow::InfoIcon, T("Violin Recording Plug-In"), T("Too many takes for last phrase! Going back to record more takes later will overwrite the last take (number 30)."), T("Ok"));

			getFilter()->increaseCurPhraseIdx();

			if (getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_FORCE)
				getFilter()->endForceCalibration();
		}
		else
		{
			// Repeat:
			bool couldIncrTakeIdx = getFilter()->incrCurPhraseTakeIdx(); // increment take index of current (just recorded) phrase

			if (!couldIncrTakeIdx)
				AlertWindow::showMessageBox(AlertWindow::InfoIcon, T("Violin Recording Plug-In"), T("Too many takes for current phrase! Any additional recordings will overwrite the last take (number 30)."), T("Ok"));
		}
	}
	else
	{
		String msg = T("Reached end of score list, but may repeat last phrase.");
		if (getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_FORCE)
			msg += T(" Ending will end force calibration.");

		if (AlertWindow::showOkCancelBox(AlertWindow::QuestionIcon, T("Violin Recording Plug-In"), msg, T("End"), T("Repeat")))
		{
			// End:
			if (getFilter()->getOperationMode() == ViolinRecordingPlugInEffect::CALIBRATING_FORCE)
				getFilter()->endForceCalibration();

			bool couldIncrTakeIdx = getFilter()->incrCurPhraseTakeIdx(); // increment take index of current (just recorded) phrase

			if (!couldIncrTakeIdx)
				AlertWindow::showMessageBox(AlertWindow::InfoIcon, T("Violin Recording Plug-In"), T("Too many takes for last phrase! Going back to record more takes later will overwrite the last take (number 30)."), T("Ok"));
		}
		else
		{
			// Repeat:
			bool couldIncrTakeIdx = getFilter()->incrCurPhraseTakeIdx(); // increment take index of current (just recorded) phrase

			if (!couldIncrTakeIdx)
				AlertWindow::showMessageBox(AlertWindow::InfoIcon, T("Violin Recording Plug-In"), T("Too many takes for current phrase! Any additional recordings will overwrite the last take (number 30)."), T("Ok"));
		}
	}

	updateEntireEditor(NULL); // update phrase idx and/or take idx, etc.
	// Note: update sync button (color) afterwards, because otherwise distracting
}


