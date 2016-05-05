// compDescfrom6DOF.c -- Polhemus Max external object.

#include <vector>
#include <string>
#include "ext.h" // Required for all Max external objects
#include "ext_obex.h"						// required for new style Max object
#include "z_dsp.h"
#include "ComputeDescriptors.hxx"
#include "AtomicEnum.hxx"
#include "AtomicFlag.hxx" 
//#include "juce.h"
//#include "ViolinRecordingPlugInEditor.hxx"
#include "utils.h"
#include "FilterFir.hxx"
#include "Derivative2.hxx"
#include "BPF.h"

#include "AsynchFileWriter.hxx"
#include "FileWriters.hxx"

#include "CBuffer.h"

#define ASSIST_OUTLET (2)
#define N_DESC 7
#define FORCE_BUFF 7
#define FORCE_BUFF_DELAY 0
#define INC_FORCE_SIZE 8
//#define MAX_NUM_VIOLINS 4
enum TrackerState
{
	TRACKER_DISCONNECTED,		// disconnected (initial state)
	TRACKER_CONNECTED,			// connected
	TRACKER_RECORDING			// connected and recording
};
AtomicEnum<TrackerState> trackerState_;

static char *pDescNames[N_DESC]=
{
	"string", "position", "bbd", "vel", "acc", "force", "tilt"
	
};//"s1x", "s1y", "s1z", "s1az", "s1el", "s1roll", "s2x", "s2y", "s2z", "s2az", "s2el", "s2roll"
//, "transformedBetas"
//"violinElevation", 	"violinAzimuth", "bowAzimuth", "bowSensor_acc", "fingerPos"
//"dforce", "ddforce", 

ComputeViolinPeformanceDescriptors computeDescriptors_;
class AsynchFileWriter *audioCh1Writer_;
class AsynchFileWriter *trackerWriter_;
const int numItemsPerFrameQualisys = 12;	
const int trackerCalibDataSize=48;
int trackerSampleRate=240;

BPF incForce, sensitForce;
ComputeSensorVelocityAndAcceleration sens2VelAndAccel_;
RawSensorData rawSensorData;

int numViolins_ =1;
LibertyTracker::ItemData violinData[MAX_NUM_VIOLINS];
LibertyTracker::ItemData bowData[MAX_NUM_VIOLINS];
int frameCount=0;

void *compDescfrom6DOF_class; // Required. Global pointing to this class
TrackerCalibration trackerCalibration_;

typedef struct _compDescfrom6DOF // Data structure for this object
{
	t_object b_ob; // Must always be the first field; used by Max
	double forceBuffer[FORCE_BUFF_DELAY+FORCE_BUFF+1];
	Atom desc[N_DESC];
	void *m_clock_compDesc;  // add a clock
	float clock_compDesc_Delay;
	void *m_clock_write;  // add a clock
	float clock_write_Delay;
	bool verbose;
	Derivative2 compBowVel_, compBowAccel_, compBowSensorVel_, compBowSensorAccel_;
	FilterFir bowVelSmoother_, bowAccelSmoother1_, bowAccelSmoother2_, bowAccelSmoother3_;
	FilterFir bowSensorVelSmoother_, bowSensorAccelSmoother1_, bowSensorAccelSmoother2_, bowSensorAccelSmoother3_;
	//bool running;
	CBuffer *circularBuffer;
	bool waitingforBow;
	Atom *transformedBetas; // array of Atoms: list
	char baseDir[MAX_PATH];
	char scoreName[MAX_PATH];
	char calibFileName[MAX_PATH];
	long ntake;
	void *descInst_out[MAX_NUM_VIOLINS];//desc_out[N_DESC]; 
	void *transformedBetas_out[MAX_NUM_VIOLINS];
} t_compDescfrom6DOF;



// Prototypes for methods: need a method for each incoming message
void *compDescfrom6DOF_new(void); // object creation method
void compDescfrom6DOF_start(t_compDescfrom6DOF *compDescfrom6DOF); // method for start message
void compDescfrom6DOF_stop(t_compDescfrom6DOF *compDescfrom6DOF); // method for start message
void compDescfrom6DOF_startRecording(t_compDescfrom6DOF *compDescfrom6DOF); // method for start message
void compDescfrom6DOF_stopRecording(t_compDescfrom6DOF *compDescfrom6DOF); // method for start message
void compDescfrom6DOF_sampleRate(t_compDescfrom6DOF *compDescfrom6DOF, double sr); 
void compDescfrom6DOF_task(t_compDescfrom6DOF *compDescfrom6DOF); //method for the scheduled task
void compDescfrom6DOF_assist(t_compDescfrom6DOF *compDescfrom6DOFr, Object *b, long msg, long arg, char *s);
void initSmoothingFilter(FilterFir &filter, int size);
void compDescfrom6DOF_6DOF(t_compDescfrom6DOF *compDescfrom6DOF, Symbol *s, short argc, t_atom *argv);
void compDescfrom6DOF_setScoreName(t_compDescfrom6DOF *compDescfrom6DOF, Symbol *s);
void compDescfrom6DOF_setDir(t_compDescfrom6DOF *compDescfrom6DOF, Symbol *s);
void compDescfrom6DOF_setTake(t_compDescfrom6DOF *compDescfrom6DOF, long ntake);
t_int *compDescfrom6DOF_perform(t_int *w);
void compDescfrom6DOF_dsp(t_compDescfrom6DOF *compDescfrom6DOF, t_signal **sp, short *count);
void compDescfrom6DOF_AsynchWrite(t_compDescfrom6DOF *compDescfrom6DOF);
void compDescfrom6DOF_setCalibFileName(t_compDescfrom6DOF *compDescfrom6DOF, Symbol *s);


void sendTrackerDataToHistoryBuffer(LibertyTracker::ItemDataIterator iter, int numTrackerFrames, int numTrackerSensors); 
void sendTrackerDataToHistoryBufferSingleFrame(const RawSensorData &frame);
bool writeHeaderFile(const char *filename,	const double str1Br[3], const double str2Br[3], const double str3Br[3], const double str4Br[3], 
														const double str1Wd[3], const double str2Wd[3], const double str3Wd[3], const double str4Wd[3], 
														const double str1Fb[3], const double str2Fb[3], const double str3Fb[3], const double str4Fb[3], 
														const double frogLhs[3], const double frogRhs[3], const double tipLhs[3], const double tipRhs[3]);
int main(void)
{
	// set up our class: create a class definition
	setup((t_messlist**) &compDescfrom6DOF_class, (method)compDescfrom6DOF_new, (method)dsp_free, (short)sizeof(t_compDescfrom6DOF), 0L, 0);
	addmess((method)compDescfrom6DOF_dsp, "dsp", A_CANT, 0);
	dsp_initclass();
	addmess((method)compDescfrom6DOF_sampleRate, "sampleRate", A_FLOAT, 0); 
	addmess((method)compDescfrom6DOF_start, "start", 0);
	addmess((method)compDescfrom6DOF_stop, "stop", 0);
	addmess((method)compDescfrom6DOF_startRecording, "startRec", 0);
	addmess((method)compDescfrom6DOF_stopRecording, "stopRec", 0);
	addmess((method)compDescfrom6DOF_assist, "assist", A_CANT, 0);
	addmess((method)compDescfrom6DOF_6DOF, "6DOF", A_GIMME, A_NOTHING);
	addmess((method)compDescfrom6DOF_setScoreName, "scoreName", A_SYM, A_NOTHING);
	addmess((method)compDescfrom6DOF_setDir, "dirBase", A_SYM, A_NOTHING);
	addmess((method)compDescfrom6DOF_setTake, "take", A_LONG, 0);
	addmess((method)compDescfrom6DOF_setCalibFileName, "calibFile", A_SYM, A_NOTHING);
		 
	//class_register(compDescfrom6DOF_class, CLASS_BOX);
}

void *compDescfrom6DOF_new(void)
{
	t_compDescfrom6DOF *compDescfrom6DOF;
	// create the new instance and return a pointer to it
	compDescfrom6DOF = (t_compDescfrom6DOF *)newobject(compDescfrom6DOF_class);
	dsp_setup((t_pxobject *)compDescfrom6DOF, 1); // left? inlet
	outlet_new((t_pxobject *)compDescfrom6DOF, "signal"); // signal outlet
	for (int i=MAX_NUM_VIOLINS-1;i>=0;i--)
		compDescfrom6DOF->transformedBetas_out[i]=listout(compDescfrom6DOF);
	//compDescfrom6DOF->descInst4_out=listout(compDescfrom6DOF);
	for (int i=MAX_NUM_VIOLINS-1;i>=0;i--)
		 compDescfrom6DOF->descInst_out[i]= listout(compDescfrom6DOF);
	


	compDescfrom6DOF->m_clock_compDesc = clock_new((t_object *)compDescfrom6DOF, (method)compDescfrom6DOF_task); //create the clock	
	compDescfrom6DOF->clock_compDesc_Delay=100;
	compDescfrom6DOF->m_clock_write = clock_new((t_object *)compDescfrom6DOF, (method)compDescfrom6DOF_AsynchWrite); //create the clock	
	compDescfrom6DOF->clock_write_Delay=100;
	compDescfrom6DOF->verbose=true;
	//compDescfrom6DOF->running=false;
	trackerState_=TRACKER_DISCONNECTED;
	compDescfrom6DOF->waitingforBow=false;

	initSmoothingFilter(compDescfrom6DOF->bowVelSmoother_, 5);
	initSmoothingFilter(compDescfrom6DOF->bowAccelSmoother1_, 5);
	initSmoothingFilter(compDescfrom6DOF->bowAccelSmoother2_, 5);
	initSmoothingFilter(compDescfrom6DOF->bowAccelSmoother3_, 9); // XXX: was 10 in Esteban code, but needs to be odd for symmetric filter

	initSmoothingFilter(compDescfrom6DOF->bowSensorVelSmoother_, 5);
	initSmoothingFilter(compDescfrom6DOF->bowSensorAccelSmoother1_, 5);
	initSmoothingFilter(compDescfrom6DOF->bowSensorAccelSmoother2_, 5);
	initSmoothingFilter(compDescfrom6DOF->bowSensorAccelSmoother3_, 9); // XXX: was 10 in Esteban code, but needs to be odd for symmetric filter
	
	//to compensate force sensitivity
	//int INC_FORCE_SIZE=8;
	float sensitForceMatrix[INC_FORCE_SIZE][2]= {{0, 0},{10,	+1.14},{20,	+0.8},{30,	+0.6},{40,	+0.68},{50,	+0.91},{60, +1.8},{67, +1.8}};
	//to compensate force with position
	float incForceMatrix[INC_FORCE_SIZE][2]= {{0, 0.47},{10,	+0.47},{20,	+0.5},{30,	+0.57},{40,	+0.65},{50,	+0.85},{60, +1.4},{67, +1.4}};
		//{{0, 0.25},{5, -0.08},{10,	-0.45},{15,	-0.77},{20,	-1.0},{25,	-1.4},{30,	-1.77},{35,	-2.17},{40,	-2.59},{45,	-2.95},{50,	-3.4},{55,	-3.9},{60, -4.6},{65, -5.12}};
	//BPF incForce
	for (int i=0;i<INC_FORCE_SIZE;i++)
		incForce.add(incForceMatrix[i][0], incForceMatrix[i][1]);
	//BPF sensitForceMatrix
	for (int i=0;i<INC_FORCE_SIZE;i++)
		sensitForce.add(sensitForceMatrix[i][0], sensitForceMatrix[i][1]);

	
	audioCh1Writer_ = NULL;
	trackerWriter_ = NULL; 
	compDescfrom6DOF->ntake=0;
	strcpy(compDescfrom6DOF->baseDir,"");
	strcpy(compDescfrom6DOF->scoreName,"");
	return(compDescfrom6DOF); // must return a pointer to the new instance
}

void compDescfrom6DOF_assist(t_compDescfrom6DOF *compDescfrom6DOF, Object *b, long msg, long arg, char *s)
{
	if (msg == ASSIST_OUTLET && arg<MAX_NUM_VIOLINS) //#define ASSIST_OUTLET (2)
	{
		sprintf(s, "Instr%d:", arg+1);		
		for (int iDesc=0;iDesc<N_DESC;iDesc++)
		{
			strcat(s,",");
			strcat(s, pDescNames[iDesc]);
		}
	}
	else if (arg>MAX_NUM_VIOLINS && arg<MAX_NUM_VIOLINS*2)
		sprintf(s, "Instr%d: Transformed betas", (arg % MAX_NUM_VIOLINS)+1);
}



void compDescfrom6DOF_startRecording(t_compDescfrom6DOF *compDescfrom6DOF)
{
	if(trackerState_ == TRACKER_CONNECTED)
	{
	// Start audio recording:
		post("Start recording...");
		if (!strcmp(compDescfrom6DOF->baseDir,""))
		{
			post("Specify dirBase!");
			return;
		}
		if (compDescfrom6DOF->ntake=0)
		{
			post("Specify take!");
			return;
		}
		if (!strcmp(compDescfrom6DOF->scoreName,""))
		{
			post("Specify scoreName!");
			return;
		}

		char fname[MAX_PATH];
		char take[4];

		//Create async writers
		int sampleRate=44100;
		int maxSecondsPerBar=2;
		float tolerance=5.0;
		audioCh1Writer_ = new AsynchFileWriter(); //(t_object *)compDescfrom6DOF);
		audioCh1Writer_->setFileWriter(WaveFileWriter(1, sampleRate));
		audioCh1Writer_->allocate(compDescfrom6DOF->clock_write_Delay, sampleRate, tolerance, maxSecondsPerBar*sampleRate);
		audioCh1Writer_->startConsumerThread();
 
		
		trackerWriter_ = new AsynchFileWriter(); //(t_object *)compDescfrom6DOF);
		trackerWriter_->setFileWriter(DatFileWriter(numItemsPerFrameQualisys*numViolins_, trackerSampleRate, 1));
		trackerWriter_->allocate(compDescfrom6DOF->clock_write_Delay, numItemsPerFrameQualisys*numViolins_*trackerSampleRate, tolerance, numItemsPerFrameQualisys*maxSecondsPerBar*trackerSampleRate);
		trackerWriter_->startConsumerThread();


		// Start audio recording:
		strcpy(fname, compDescfrom6DOF->baseDir);	
		strcat(fname, "\\");
		strcat(fname, compDescfrom6DOF->scoreName);
		strcat(fname, "-");
		sprintf(take, "%03d", compDescfrom6DOF->ntake);
		strcat(fname, take);
		strcat(fname, "-ch1.wav");
		//char *audioCh1Filename = "testch1.wav";
		audioCh1Writer_->postStartDiskWriteEvent((const char *)fname, 0);

		// Start tracker recording:
		strcpy(fname, compDescfrom6DOF->baseDir);	
		strcat(fname, "\\");
		strcat(fname, compDescfrom6DOF->scoreName);
		strcat(fname, "-");
		sprintf(take, "%03d", compDescfrom6DOF->ntake);
		strcat(fname, take);
		strcat(fname, "-tracker.dat");
		//char *trackerFilename = "testtracker.dat";
		trackerWriter_->postStartDiskWriteEvent((const char *)fname, 0);
	// Write header file (raw binary):
	// NOTE: This file is written synchronously (to reduce code size), but 
	// it is only few data.
		//char *headerFilename = "testheader.dat";
		strcpy(fname, compDescfrom6DOF->baseDir);	
		strcat(fname, "\\");
		strcat(fname, compDescfrom6DOF->scoreName);
		strcat(fname, "-");
		sprintf(take, "%03d", compDescfrom6DOF->ntake);
		strcat(fname, take);
		strcat(fname, "-header.dat");
		int currViolin=0;
		if (!writeHeaderFile(fname,
			trackerCalibration_.getBeta(currViolin, TrackerCalibration::STR1_BRIDGE).getPtr(),
			trackerCalibration_.getBeta(currViolin, TrackerCalibration::STR2_BRIDGE).getPtr(), 
			trackerCalibration_.getBeta(currViolin, TrackerCalibration::STR3_BRIDGE).getPtr(), 
			trackerCalibration_.getBeta(currViolin, TrackerCalibration::STR4_BRIDGE).getPtr(), 
			trackerCalibration_.getBeta(currViolin, TrackerCalibration::STR1_WOOD).getPtr(), 
			trackerCalibration_.getBeta(currViolin, TrackerCalibration::STR2_WOOD).getPtr(), 
			trackerCalibration_.getBeta(currViolin, TrackerCalibration::STR3_WOOD).getPtr(), 
			trackerCalibration_.getBeta(currViolin, TrackerCalibration::STR4_WOOD).getPtr(), 
			trackerCalibration_.getBeta(currViolin, TrackerCalibration::STR1_FB).getPtr(), 
			trackerCalibration_.getBeta(currViolin, TrackerCalibration::STR2_FB).getPtr(), 
			trackerCalibration_.getBeta(currViolin, TrackerCalibration::STR3_FB).getPtr(), 
			trackerCalibration_.getBeta(currViolin, TrackerCalibration::STR4_FB).getPtr(), 
			trackerCalibration_.getBeta(currViolin, TrackerCalibration::BOW_FROG_LHS).getPtr(), 
			trackerCalibration_.getBeta(currViolin, TrackerCalibration::BOW_FROG_RHS).getPtr(), 
			trackerCalibration_.getBeta(currViolin, TrackerCalibration::BOW_TIP_LHS).getPtr(), 
			trackerCalibration_.getBeta(currViolin, TrackerCalibration::BOW_TIP_RHS).getPtr()))
		{
			post("[r]ERROR: Failed writing recording header file!");
		}
		trackerState_ = TRACKER_RECORDING;
		clock_fdelay(compDescfrom6DOF->m_clock_write, compDescfrom6DOF->clock_write_Delay);
	}
}


bool writeHeaderFile(const char *filename,	const double str1Br[3], const double str2Br[3], const double str3Br[3], const double str4Br[3], 
														const double str1Wd[3], const double str2Wd[3], const double str3Wd[3], const double str4Wd[3], 
														const double str1Fb[3], const double str2Fb[3], const double str3Fb[3], const double str4Fb[3], 
														const double frogLhs[3], const double frogRhs[3], const double tipLhs[3], const double tipRhs[3])
	{	
		std::ofstream headerFile;
		headerFile.open(filename, std::ios_base::trunc | std::ios_base::binary);

		if (!headerFile.is_open())
			return false;
		// File format version 2 and above:
		// Four character ID:
		char id[4];
		id[0] = 'V'; // Violin
		id[1] = 'R'; // Recording
		id[2] = 'H'; // Header
		id[3] = 'X'; // version 2 and above
		headerFile.write(&id[0], sizeof(char));
		headerFile.write(&id[1], sizeof(char));
		headerFile.write(&id[2], sizeof(char));
		headerFile.write(&id[3], sizeof(char));

		int fileFormatVersion = 2; //int32_t
		headerFile.write((const char *)&fileFormatVersion, sizeof(int));

		// Note:
		// Sample rates are already stored in the audio/tracker files themselves.
		// Input latencies are not important as output from plug-in is supposed to be synchronized.
		// Host metronome information is stored in a easier-to-change/read text format.

		// Calibration data:
		headerFile.write((const char *)str1Br, 3*sizeof(double));
		headerFile.write((const char *)str2Br, 3*sizeof(double));
		headerFile.write((const char *)str3Br, 3*sizeof(double));
		headerFile.write((const char *)str4Br, 3*sizeof(double));

		headerFile.write((const char *)str1Wd, 3*sizeof(double));
		headerFile.write((const char *)str2Wd, 3*sizeof(double));
		headerFile.write((const char *)str3Wd, 3*sizeof(double));
		headerFile.write((const char *)str4Wd, 3*sizeof(double));

		headerFile.write((const char *)str1Fb, 3*sizeof(double));
		headerFile.write((const char *)str2Fb, 3*sizeof(double));
		headerFile.write((const char *)str3Fb, 3*sizeof(double));
		headerFile.write((const char *)str4Fb, 3*sizeof(double));

		headerFile.write((const char *)frogLhs, 3*sizeof(double));
		headerFile.write((const char *)frogRhs, 3*sizeof(double));

		headerFile.write((const char *)tipLhs, 3*sizeof(double));
		headerFile.write((const char *)tipRhs, 3*sizeof(double));

		headerFile.close();
		return true;
}

void compDescfrom6DOF_AsynchWrite(t_compDescfrom6DOF *compDescfrom6DOF)
{	
	if (audioCh1Writer_!=NULL)
	{
		audioCh1Writer_->timerCallback();
		trackerWriter_->timerCallback();
		//post("timer triggered");
		clock_fdelay(compDescfrom6DOF->m_clock_write, compDescfrom6DOF->clock_write_Delay); //schedule the clock again
	}
}

void compDescfrom6DOF_stopRecording(t_compDescfrom6DOF *compDescfrom6DOF)
{
	if (trackerState_ == TRACKER_RECORDING)
	{
		// Stop audio/tracker/arduino recording:
		trackerState_ = TRACKER_CONNECTED;
		audioCh1Writer_->postStopDiskWriteEvent();
		audioCh1Writer_->stopConsumerThread(); // (blocking)
		delete audioCh1Writer_;
		audioCh1Writer_ = NULL;
		trackerWriter_->postStopDiskWriteEvent();
		trackerWriter_->stopConsumerThread(); // (blocking)
		delete trackerWriter_;
		trackerWriter_ = NULL;
	post("Stop recording");
	}

}

void compDescfrom6DOF_start(t_compDescfrom6DOF *compDescfrom6DOF)
{
	post("Start reading 6DOF...");		
	
	//Load 6RigidBody XML file from Qualisys software
	//trackerCalibration_.init(1);
	bool ok = trackerCalibration_.loadFrom6DOFXMLFile(compDescfrom6DOF->calibFileName);
	if (ok)
	{
		post("Calibration file loaded correctly");

		if (!compDescfrom6DOF->clock_compDesc_Delay)
			compDescfrom6DOF->clock_compDesc_Delay=100;

		trackerState_=TRACKER_CONNECTED;
		numViolins_=trackerCalibration_.getNumberViolins();
		float consumptionInterval=compDescfrom6DOF->clock_compDesc_Delay/1000;
		float prodConsRate= 2*numViolins_*trackerSampleRate;
		float tolerance=10.0;
		
		//What if start is pressed twice? CBUFFER must be deleted.
		
		compDescfrom6DOF->circularBuffer=new CBuffer(consumptionInterval*prodConsRate*tolerance);
		compDescfrom6DOF->transformedBetas = new Atom[trackerCalibDataSize*numViolins_];
	
		clock_fdelay(compDescfrom6DOF->m_clock_compDesc, compDescfrom6DOF->clock_compDesc_Delay);
	}
	else
	{
		post("WARNING: Calibration file failed to load.");
		//trackerCalibrationState_ = TRACKER_NOT_CALIBRATED;
		// (hasCalibrationBeenModified_ remains true if was true)
	}
	
}
void compDescfrom6DOF_stop(t_compDescfrom6DOF *compDescfrom6DOF)
{
	//compDescfrom6DOF->running=false;
	trackerState_=TRACKER_DISCONNECTED;
	compDescfrom6DOF->circularBuffer->resetIdxs();
}


void compDescfrom6DOF_6DOF(t_compDescfrom6DOF *compDescfrom6DOF, Symbol *s, short argc, t_atom *argv)
{
	if (trackerState_==TRACKER_DISCONNECTED)
		return;
	if (argc < 7 || argc > 8)
	{ //post("6DOF must be 6 floats, received %d", argc);
		return;
	}
	
	if (argv[0].a_type == A_SYM)
	{	
		//post("simbolo: %s", argv[0].a_w.w_sym->s_name);
		//parse string. If it is /qtm/6d means 6dof data
		char *sixDOFStr="/qtm/6d_euler/";
		char *frameNumStr="/qtm/data";
		char *beginning=strstr(argv[0].a_w.w_sym->s_name, frameNumStr);;
		if (beginning!=NULL)
		{
			//post("data: %s", beginning);
			if(frameCount!=0) //if its not the first frame, save previous data to circBuffer and reset violin and BowData.
			{//save last frame data
				if (compDescfrom6DOF->circularBuffer->getSize()-compDescfrom6DOF->circularBuffer->getCount()<2) //cbIsFull())
					post("Overwritting frames in circular buffer: data overrun.");
				else
				{
					compDescfrom6DOF->circularBuffer->cbWrite(&violinData[0]);
					compDescfrom6DOF->circularBuffer->cbWrite(&bowData[0]);
					compDescfrom6DOF->circularBuffer->cbWrite(&violinData[1]);
					compDescfrom6DOF->circularBuffer->cbWrite(&bowData[1]);
				}
				//prepare new data
				for (int i=0;i<numViolins_;i++)
				{
					violinData[i]=LibertyTracker::ItemData();
					violinData[i].frameCount=argv[4].a_w.w_long;
					bowData[i]=LibertyTracker::ItemData();
					bowData[i].frameCount=argv[4].a_w.w_long;
				}			
				frameCount++;
				if(argv[4].a_w.w_long != frameCount)
				{	post("WARNING: OSC last frameNumber=%d, actual frameNumber=%d",frameCount-1, argv[4].a_w.w_long);
					frameCount=argv[4].a_w.w_long;
				}
			}
			else //its the first arriving frame
				frameCount=argv[4].a_w.w_long;
		}
		
		beginning=strstr(argv[0].a_w.w_sym->s_name, sixDOFStr);
		if (beginning==NULL) return;
		for(int iLabel=0;iLabel<numViolins_;iLabel++)
		{
			if (strcmp(beginning+strlen(sixDOFStr),trackerCalibration_.getLabels()[iLabel].c_str())==0)
			{
				violinData[iLabel].position[0]= argv[1].a_w.w_float/10;
				violinData[iLabel].position[1]= argv[2].a_w.w_float/10;
				violinData[iLabel].position[2]= argv[3].a_w.w_float/10;
				violinData[iLabel].orientation[0]= argv[4].a_w.w_float;
				violinData[iLabel].orientation[1]= argv[5].a_w.w_float;
				violinData[iLabel].orientation[2]= argv[6].a_w.w_float;		
				break;
				//post("received 6DOF Violin: %f,%f,%f,%f,%f,%f... waiting for bow,", newData.position[0],newData.position[1],newData.position[2],newData.orientation[0],newData.orientation[1],newData.orientation[2]);
			}
			else if(strcmp(beginning+strlen(sixDOFStr),trackerCalibration_.getBowLabels()[iLabel].c_str())==0)
			{
				bowData[iLabel].position[0]= argv[1].a_w.w_float/10;
				bowData[iLabel].position[1]= argv[2].a_w.w_float/10;
				bowData[iLabel].position[2]= argv[3].a_w.w_float/10;
				bowData[iLabel].orientation[0]= argv[4].a_w.w_float;
				bowData[iLabel].orientation[1]= argv[5].a_w.w_float;
				bowData[iLabel].orientation[2]= argv[6].a_w.w_float;
				break;
				//post("received 6DOF Bow: %f,%f,%f,%f,%f,%f... waiting for violin,", newData.position[0],newData.position[1],newData.position[2],newData.orientation[0],newData.orientation[1],newData.orientation[2]);
			}
		}
		
	}
}

void compDescfrom6DOF_sampleRate(t_compDescfrom6DOF *compDescfrom6DOF, double sr)
{
	compDescfrom6DOF->clock_compDesc_Delay=sr;
}

void compDescfrom6DOF_dsp(t_compDescfrom6DOF *compDescfrom6DOF, t_signal **sp, short *count)
{/*The first parameter in dsp_add() is the name of your perform method, the second
number indicates the number of arguments in the perform method, followed by the
arguments, all of which must be the size of a pointer or a long and appears in your
perform method as an array of t_int. The second parameter passed to the dsp
method, t_signal **sp, is an array of pointers to struct t_signal
 */
	post("compDescfrom6DOF~ sr: %f", sp[0]->s_sr);
	post("compDescfrom6DOF~ size of buffer: %d", sp[0]->s_n);
	//sp[0]->s_sr=240;
	//sp[0]->s_n=240;
	dsp_add(compDescfrom6DOF_perform, 3, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
		 //3, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}


t_int *compDescfrom6DOF_perform(t_int *w)
{
	//t_reverbObj *reverbObj = (t_reverbObj *)(w[1]);
    t_float *in = (t_float *)(w[1]);
    t_float *out = (t_float *)(w[2]);
    int n = (int)(w[3]);
	int m=n;
	t_float *auxIn=in;
	while (m--)
		*out++ = *in++;
	//post("llamando perform");
	if(trackerState_ == TRACKER_RECORDING)
	{
		// Send to history buffer:
		//post("recording audioBuffer");
		int result=audioCh1Writer_->writeData(auxIn, n); //audioBuffer.getSampleData(0), audioBuffer.getNumSamples());
		//post("result=%d",result);
		if (result==0) 
			post("data not correctly saved");
	}

	return(w + 4); // always add one more than the 2nd argument in dsp_add()
}

//function that will do something when the clock is executed
void compDescfrom6DOF_task(t_compDescfrom6DOF *compDescfrom6DOF)
{		
	const int numTrackerItems = compDescfrom6DOF->circularBuffer->getCount(); //tracker_.queryFrames(beginBuffer);
	const int numTrackerSensors =numViolins_*2; //tracker_.getNumEnabledSensors(); // num items per frame
	const int numTrackerFrames =  numTrackerItems/numTrackerSensors;
	const int readIdx=compDescfrom6DOF->circularBuffer->getRIdx();
	const int bufferSize=compDescfrom6DOF->circularBuffer->getSize();

	if (trackerState_==TRACKER_DISCONNECTED) return; //(compDescfrom6DOF->running==false) return;
	if (numTrackerFrames==0)
	{
		clock_fdelay(compDescfrom6DOF->m_clock_compDesc, compDescfrom6DOF->clock_compDesc_Delay); //schedule the clock again
		//post("No data found in buffer");
		return;
	}

	ViolinPerformanceDescriptors descriptors;
	LibertyTracker::ItemDataIterator beginBuffer(compDescfrom6DOF->circularBuffer->cbGetBuffer(), readIdx, bufferSize);
	//advance circular buffer
	LibertyTracker::ItemData elem;
	for (int i=0; i<numTrackerItems;i++)
		compDescfrom6DOF->circularBuffer->cbRead(&elem);

	//if (trackerState_==TRACKER_RECORDING)
	//{
	//	//call function to record polhemus
	//	sendTrackerDataToHistoryBuffer(beginBuffer, numTrackerFrames, numTrackerSensors);
	//}

	for (int i = 0; i < numTrackerFrames; ++i)
	//	int i=numTrackerFrames-1;
	{
		// Compute 'raw' descriptors for current frame:
		rawSensorData = computeDescriptors_.trackerDataToRawSensorData(beginBuffer,numViolins_,0);

		if (trackerState_==TRACKER_RECORDING)
		{
			//call function to record polhemus
			//sendTrackerDataToHistoryBuffer(beginBuffer, numTrackerFrames, numTrackerSensors);
			// Send current frame to history buffer
			sendTrackerDataToHistoryBufferSingleFrame(rawSensorData);
		}
		//bool useAutoString=true;
		bool isCalibratingForce=false;
		bool isAutoStringEnabled_=true;
		CalibrationAngles anglesCalibration_; //anglesCalibration_ NOT USED ANYMORE. TODO:remove it
		
		for (int iViolin=0; iViolin<numViolins_; iViolin++)
		{
			Derived3dData derived3dData = computeDescriptors_.computeDerived3dData(rawSensorData, trackerCalibration_, isAutoStringEnabled_, anglesCalibration_, isCalibratingForce, NULL, iViolin);
			// XXX: above descriptors are computed twice
			descriptors = computeDescriptors_.computeViolinPerformanceDescriptors(rawSensorData, derived3dData);
			// Next frame:
			beginBuffer.advance(numTrackerSensors);				

			//Compute descriptors. Do it for all received frames??
			float bowVel=0, bowVelSmooth=0;
			float bowAccel=0, bowAccelSmooth=0;	
			for(int i=0;i<N_DESC;i++)
			{				
				if (!strcmp(pDescNames[i],"string"))
					SETLONG(&compDescfrom6DOF->desc[i], (long)derived3dData.playedString);
				else if (!strcmp(pDescNames[i],"position"))
					SETFLOAT(&compDescfrom6DOF->desc[i],descriptors.bowDisplacement);
				else if (!strcmp(pDescNames[i],"bbd"))
					SETFLOAT(&compDescfrom6DOF->desc[i],descriptors.bowBridgeDistance);
				else if (!strcmp(pDescNames[i],"vel"))
				{
					//compDescfrom6DOF->desc[i]=descriptors.bowVel;
					bowVel = compDescfrom6DOF->compBowVel_.compute((float)descriptors.bowDisplacement, 240.0f/numTrackerFrames);			
					compDescfrom6DOF->bowVelSmoother_.process(&bowVel, &bowVelSmooth, 1);
					SETFLOAT(&compDescfrom6DOF->desc[i], bowVelSmooth);
				}
				else if (!strcmp(pDescNames[i],"acc"))
				{	//compDescfrom6DOF->desc[i]=descriptors.bowAccel;
					bowAccel = compDescfrom6DOF->compBowAccel_.compute(bowVel, 240.0f/numTrackerFrames);
					compDescfrom6DOF->bowAccelSmoother1_.process(&bowAccel, &bowAccelSmooth, 1);
					compDescfrom6DOF->bowAccelSmoother2_.process(&bowAccelSmooth, &bowAccelSmooth, 1);
					compDescfrom6DOF->bowAccelSmoother3_.process(&bowAccelSmooth, &bowAccelSmooth, 1);
					SETFLOAT(&compDescfrom6DOF->desc[i], bowAccelSmooth);
				}
				else if (!strcmp(pDescNames[i],"force")) 
				{
					float forceCorrected=descriptors.bowForce+incForce.get(descriptors.bowDisplacement);
					forceCorrected=forceCorrected/sensitForce.get(descriptors.bowDisplacement)*2.5;
					forceCorrected=MIN(forceCorrected,3);
					forceCorrected=MAX(forceCorrected,-0.5);
					SETFLOAT(&compDescfrom6DOF->desc[i], forceCorrected);
					for (int j=0;j<FORCE_BUFF+FORCE_BUFF_DELAY;j++)
					{
						compDescfrom6DOF->forceBuffer[j]=compDescfrom6DOF->forceBuffer[j]+1;
					}
					compDescfrom6DOF->forceBuffer[FORCE_BUFF+FORCE_BUFF_DELAY]=forceCorrected; //descriptors.bowForce;
				}
				//outlet_float(compDescfrom6DOF->desc_out[i], compDescfrom6DOF->desc[i]);
				outlet_list(compDescfrom6DOF->descInst_out[iViolin], (t_symbol *)"list", N_DESC, compDescfrom6DOF->desc);
			}
			//transformed Betas		
			SETFLOAT(&compDescfrom6DOF->transformedBetas[0], derived3dData.posStr1Bridge(0,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[1], derived3dData.posStr1Bridge(1,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[2], derived3dData.posStr1Bridge(2,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[3], derived3dData.posStr2Bridge(0,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[4], derived3dData.posStr2Bridge(1,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[5], derived3dData.posStr2Bridge(2,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[6], derived3dData.posStr3Bridge(0,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[7], derived3dData.posStr3Bridge(1,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[8], derived3dData.posStr3Bridge(2,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[9], derived3dData.posStr4Bridge(0,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[10], derived3dData.posStr4Bridge(1,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[11], derived3dData.posStr4Bridge(2,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[12], derived3dData.posStr1Wood(0,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[13], derived3dData.posStr1Wood(1,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[14], derived3dData.posStr1Wood(2,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[15], derived3dData.posStr2Wood(0,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[16], derived3dData.posStr2Wood(1,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[17], derived3dData.posStr2Wood(2,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[18], derived3dData.posStr3Wood(0,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[19], derived3dData.posStr3Wood(1,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[20], derived3dData.posStr3Wood(2,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[21], derived3dData.posStr4Wood(0,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[22], derived3dData.posStr4Wood(1,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[23], derived3dData.posStr4Wood(2,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[24], derived3dData.posStr1Fb(0,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[25], derived3dData.posStr1Fb(1,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[26], derived3dData.posStr1Fb(2,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[27], derived3dData.posStr2Fb(0,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[28], derived3dData.posStr2Fb(1,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[29], derived3dData.posStr2Fb(2,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[30], derived3dData.posStr3Fb(0,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[31], derived3dData.posStr3Fb(1,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[32], derived3dData.posStr3Fb(2,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[33], derived3dData.posStr4Fb(0,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[34], derived3dData.posStr4Fb(1,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[35], derived3dData.posStr4Fb(2,0));

			SETFLOAT(&compDescfrom6DOF->transformedBetas[36], derived3dData.posBowFrogLhs(0,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[37], derived3dData.posBowFrogLhs(1,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[38], derived3dData.posBowFrogLhs(2,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[39], derived3dData.posBowFrogRhs(0,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[40], derived3dData.posBowFrogRhs(1,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[41], derived3dData.posBowFrogRhs(2,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[42], derived3dData.posBowTipLhs(0,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[43], derived3dData.posBowTipLhs(1,0)); 
			SETFLOAT(&compDescfrom6DOF->transformedBetas[44], derived3dData.posBowTipLhs(2,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[45], derived3dData.posBowTipRhs(0,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[46], derived3dData.posBowTipRhs(1,0));
			SETFLOAT(&compDescfrom6DOF->transformedBetas[47], derived3dData.posBowTipRhs(2,0));
			outlet_list(compDescfrom6DOF->transformedBetas_out[iViolin], (t_symbol *)"list", trackerCalibDataSize, compDescfrom6DOF->transformedBetas);
		} 
	}
	//post("task done....");
	if (trackerState_==TRACKER_CONNECTED || trackerState_==TRACKER_RECORDING) //compDescfrom6DOF->running==true)
		clock_fdelay(compDescfrom6DOF->m_clock_compDesc, compDescfrom6DOF->clock_compDesc_Delay); //schedule the clock again
}


void sendTrackerDataToHistoryBuffer(LibertyTracker::ItemDataIterator iter, int numTrackerFrames, int numTrackerSensors)
{
	// Process tracker frames:
	for (int i = 0; i < numTrackerFrames; ++i)
	{
		// Compute 'raw' descriptors for current frame:
		RawSensorData rawSensorData = computeDescriptors_.trackerDataToRawSensorData(iter, 1, 0);

		// Compute frame count of frame:
		frameCount = iter.item().frameCount; // don't offset first frame to avoid not having a lhs frame for interpolation

		bool dropCurFrame = false;


		// Use current frame if it's not dropped:
		if (!dropCurFrame)
		{
			// Send current frame to history buffer
			sendTrackerDataToHistoryBufferSingleFrame(rawSensorData);
		}

		// Next frame:
		iter.advance(numTrackerSensors);
	}
}

void sendTrackerDataToHistoryBufferSingleFrame(const RawSensorData &frame)
{
	static float trackerFrame[numItemsPerFrameQualisys];

	// sensor 1 (violin body):
	trackerFrame[0] = (float)frame.violinBodySensPos[0](0, 0);
	trackerFrame[1] = (float)frame.violinBodySensPos[0](1, 0);
	trackerFrame[2] = (float)frame.violinBodySensPos[0](2, 0);
	trackerFrame[3] = (float)frame.violinBodySensOrientation[0](0, 0);
	trackerFrame[4] = (float)frame.violinBodySensOrientation[0](1, 0);
	trackerFrame[5] = (float)frame.violinBodySensOrientation[0](2, 0);
 
	// sensor 2 (bow):
	trackerFrame[6] = (float)frame.bowSensPos[0](0, 0);
	trackerFrame[7] = (float)frame.bowSensPos[0](1, 0);
	trackerFrame[8] = (float)frame.bowSensPos[0](2, 0);
	trackerFrame[9] = (float)frame.bowSensOrientation[0](0, 0);
	trackerFrame[10] = (float)frame.bowSensOrientation[0](1, 0);
	trackerFrame[11] = (float)frame.bowSensOrientation[0](2, 0);

	int n=trackerWriter_->writeData(trackerFrame, numItemsPerFrameQualisys); // (internally checks and logs if not all of the requested items could be written)
	int a=n;// Note: Write entire frame as a single atomic operation.
}

void compDescfrom6DOF_setDir(t_compDescfrom6DOF *compDescfrom6DOF, Symbol *s)
{
	strcpy(compDescfrom6DOF->baseDir,s->s_name);
	if (compDescfrom6DOF->verbose)
		post("dirBase=%s",compDescfrom6DOF->baseDir);	
}

void compDescfrom6DOF_setScoreName(t_compDescfrom6DOF *compDescfrom6DOF, Symbol *s)
{
	strcpy(compDescfrom6DOF->scoreName,s->s_name);
	if (compDescfrom6DOF->verbose)
		post("scoreName=%s",compDescfrom6DOF->scoreName);	
}

void compDescfrom6DOF_setTake(t_compDescfrom6DOF *compDescfrom6DOF, long ntake)
{
	compDescfrom6DOF->ntake=ntake;
}

void compDescfrom6DOF_setCalibFileName(t_compDescfrom6DOF *compDescfrom6DOF, Symbol *s)
{
	strcpy(compDescfrom6DOF->calibFileName, s->s_name);
	if (compDescfrom6DOF->verbose)
		post("calibFileName=%s",compDescfrom6DOF->calibFileName);	
}

void initSmoothingFilter(FilterFir &filter, int size)
{
	if (size <= 0)
		return;

	float *coeffs = new float[size];

	// compute gaussian window for n points:
	computeGaussWindow(coeffs, size);

	// scale so coefficients sum to unity:
	float sum = 0.0f;
	for (int i = 0; i < size; ++i)
		sum += coeffs[i];
	for (int i = 0; i < size; ++i)
		coeffs[i] /= sum;

	// init fir filter:
	filter.init(coeffs, size); // copies coeffs to internal memory

	delete[] coeffs;
}
