#ifndef INCLUDED_VIOLINRECORDINGPLUGINEDITOR_HXX
#define INCLUDED_VIOLINRECORDINGPLUGINEDITOR_HXX

#include "ViolinRecordingPlugInConfig.hxx"

#include "ViolinRecordingPlugInEffect.hxx"
#include "ChangeListenerBinder.hxx"
#include "GraphPlotter.hxx"
#include "SimpleMatrix.hxx"
#include "LockFreeFifo.hxx"
#include "AtomicPtr.hxx"

#include "Scene3d.hxx"

#include "ViolinRecordingPlugInConfig.hxx"
#if (DISABLE_TIMERS != 0)
#include "DummyTimer.hxx"
#define Timer DummyTimer
#endif

class ViolinRecordingPlugInEditor :	public AudioProcessorEditor,
									public FilenameComponentListener,
									public ButtonListener,
									public ListBoxModel,
									public TextEditorListener,
									public ComboBoxListener,
									public SliderListener,
									private Timer
{
public:
    ViolinRecordingPlugInEditor(ViolinRecordingPlugInEffect *const ownerFilter);
    ~ViolinRecordingPlugInEditor();

	void filenameComponentChanged(FilenameComponent *filenameComponent);

	void buttonClicked(Button *button);

	int getNumRows();
	void paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected);

	void textEditorTextChanged(TextEditor &editor) {}
	void textEditorReturnKeyPressed(TextEditor &editor);
	void textEditorEscapeKeyPressed(TextEditor &editor);
	void textEditorFocusLost(TextEditor &editor);

	void comboBoxChanged(ComboBox *comboBoxThatHasChanged);

	void sliderValueChanged(Slider *slider);

	void paint(Graphics& g);

private:
	// convenience function
    ViolinRecordingPlugInEffect *getFilter() const throw()
	{
		return (ViolinRecordingPlugInEffect *)getAudioProcessor();
	}

	// Tooltip window:
	TooltipWindow tooltipWindow;

	// Widgets:
	ToggleButton *useGagesButton_;
	ComboBox *comPortComboBox_;
	ToggleButton *useStereoButton_;
	ToggleButton *useAutoStringButton_;
	ToggleButton *useScene3dButton_;

	TextButton *connectTrackerButton_;
	TextButton *disconnectTrackerButton_;
	Label *trackerStatusLabel_;
	Label *calibrationStatusLabel_;

	TextButton *calibrateButton_;
	TextButton *loadCalibrationButton_;
	TextButton *saveCalibrationButton_;
	TextButton *calibrateForceButton_;

	TextButton *calibrationTakeSampleButton_;
	TextButton *calibrationPrevStepButton_;
	TextButton *calibrationNextStepButton_;
	TextButton *calibrationStopButton_;

	FilenameComponent *scoreListFileChooser_;
	FilenameComponent *outputPathChooser_;

	ListBox *trackerLogListBox_;
	TextButton *clearEventLogButton_;

	Label *phraseLabel_;
	Label *phraseIndexLabel_;
	TextEditor *phraseNameEditor_;
	Label *takeIndexLabel_;
	TextButton *prevPhraseButton_;
	TextButton *nextPhraseButton_;

	Label *bowForceLabel_;
	Label *summedSensorAccelerationLabel_;

	ToggleButton *manualSyncButton_;
	Slider *trackerToAudioSyncOffsetSlider_;
	Slider *arduinoToTrackerSyncOffsetSlider_;
	ToggleButton *plotSyncSigButton_;
//t	ToggleButton *contRetriggerButton_;
	TextButton *triggerSyncButton_;

	ToggleButton *tuningRefToneButton_;

	TextButton *makeModelButton_;
	TextButton *loadModelButton_;
	TextButton *resetCameraButton_;

	// Score list update check:
	Time lastCheckedScoreListModTime_;
	CriticalSection scoreListLock_;
	void timerCallback();

	// Effect processing activity check:
	bool isEffectProcessingSuspended_;
	int64 lastProcessingDeadFlagSetTime_;

	// Misc.:
	int isUsingGages_; // 0: false, 1: true, -1: unknown

public:
	AtomicFlag arePlotGraphsValid_;
	GraphPlotter *bowDisplacementPlot_;
	GraphPlotter *bowForcePlot_;
	GraphPlotter *bowBridgeDistancePlot_;
	GraphPlotter *noiseAndSyncPlot_;

	AtomicFlag isScene3dValid_;
	Scene3d *scene3d_;

private:
	void updateEntireEditor(void *);
	void updateLoggerListBoxAsynchronous(void *);

	void updateEditorSize();
	void updateConfigurationSection();
	void updateTrackerConnectSection();
	void updateCalibrationSection();
	void updateChoosers();
	void updatePhraseSection();
	void updateScene3dSection();
	void updateSyncSection();

	ChangeListenerBinder<ViolinRecordingPlugInEditor> updateEntireEditorSlot_;
	ChangeListenerBinder<ViolinRecordingPlugInEditor> updateLoggerListBoxAsynchronouslySlot_;
	ChangeListenerBinder<ViolinRecordingPlugInEditor> endTrackerCalibrationSequenceReachedSlot_;
	ChangeListenerBinder<ViolinRecordingPlugInEditor> endForceCalibrationSequenceReachedSlot_;
	ChangeListenerBinder<ViolinRecordingPlugInEditor> endSampling3dModelSequenceReachedSlot_;
	ChangeListenerBinder<ViolinRecordingPlugInEditor> endRecordingPhraseSlot_;

	void handleExternalScoreListUpdates();

	void startTrackerCalibration();
	void stopTrackerCalibration(bool successful);
	void endTrackerCalibrationSequenceReached(void *);
	void trackerCalibrationSequenceCanceled();

	void startForceCalibration();
	void endForceCalibrationSequenceReached(void *);
	void doSaveForceCalibrationDialog();

	void endSampling3dModelSequenceReached(void *);
	void doSave3dModelDialog();

	enum CalibrationSectionMode
	{
        NORMAL,
		CALIBRATING // calibrating tracker or force
	};

	void switchCalibrationSection(CalibrationSectionMode mode);

	void doEndRecordingPhraseDialog(void *);
};


#endif
