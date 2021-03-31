#include <rtxi-lfpRatiometer.h>

using namespace std;

extern "C" Plugin::Object *createRTXIPlugin(void){
    return new rtxilfpRatiometer();
}

static DefaultGUIModel::variable_t vars[] = {
{ 
  "Time Window (s)", "Time Window (s)", 
  DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE
  },
{ 
  "Sampling Rate (Hz)", "Sampling Rate (Hz)", 
  DefaultGUIModel::STATE | DefaultGUIModel::DOUBLE,
  },
{ 
  "LF Lower Bound", "LF Lower Bound",
  DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,
  },
{ 
  "LF Upper Bound", "LF Upper Bound",
  DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,
  },
{ 
  "HF Lower Bound", "HF Lower Bound",
  DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,
  },
{ 
  "HF Upper Bound", "HF Upper Bound",
  DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,
  },  
{
    "input_LFP", "Input LFP",
    DefaultGUIModel::INPUT | DefaultGUIModel::DOUBLE,
  },
{
    "ratio", "Output LFP Power Ratio",
    DefaultGUIModel::OUTPUT | DefaultGUIModel::DOUBLE,
  },
{
   "debug var", "debug var",
   DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE,
}
};

static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

// defining what's in the object's constructor
// sampling set by RT period
rtxilfpRatiometer::rtxilfpRatiometer(void) :
DefaultGUIModel("lfpRatiometer with Custom GUI", ::vars, ::num_vars),
period(((double)RT::System::getInstance()->getPeriod())*1e-9), // grabbing RT period
sampling(1.0/period), // calculating RT sampling rate
lfpratiometer(N, sampling) // constructing lfpRatiometer object
{
    setWhatsThis("<p><b>lfpRatiometer:</b><br>Given an input, this module calculates the LF/HF ratio over a specified causal time window.</p>");
    DefaultGUIModel::createGUI(vars, num_vars);
    customizeGUI();
    update(INIT);
    refresh();
    QTimer::singleShot(0, this, SLOT(resizeMe()));
}

// defining what's in the object's destructor
rtxilfpRatiometer::~rtxilfpRatiometer(void) { }

// real-time RTXI function
void rtxilfpRatiometer::execute(void) {

  // push new time series reading to lfpRatiometer
  lfpratiometer.pushTimeSample(input(0));

  // calculate LF/HF ratio
  lfpratiometer.calcRatio();

  // put the LF/HF ratio into the output
  output(0) = lfpratiometer.getRatio();
    
}

// update function (not running in real time)
void rtxilfpRatiometer::update(DefaultGUIModel::update_flags_t flag)
{
  switch (flag) {
    case INIT:
      setParameter("Time Window (s)", sampling/N);
      setState("Sampling Rate (Hz)", sampling);
      // get bounds from lfpratiometer object
      setParameter("LF Lower Bound", lfpratiometer.getFreqBounds()[0]);
      setParameter("LF Upper Bound", lfpratiometer.getFreqBounds()[1]);
      setParameter("HF Lower Bound", lfpratiometer.getFreqBounds()[2]);
      setParameter("HF Upper Bound", lfpratiometer.getFreqBounds()[3]);

      setParameter("debug var", window_tracker_dummy);
      break;

    case MODIFY:
      // defining parameters needed for constructor
      sampling = getState("Sampling Rate (Hz)").toDouble();
      N = (int) (getParameter("Time Window (s)").toDouble() * sampling);

      // making new FFT plan
      lfpratiometer.changeFFTPlan(N, sampling);

      // setting frequency bounds based on user input
      lfpratiometer.setRatioParams(getParameter("LF Lower Bound").toDouble(),
          getParameter("LF Upper Bound").toDouble(),
          getParameter("HF Lower Bound").toDouble(),
          getParameter("HF Upper Bound").toDouble());
      
      // setting DFT windowing function choice
      if (windowShape->currentIndex() == 1) {
        lfpratiometer.window_rect();
        window_tracker_dummy = 1;
      }
      else if (windowShape->currentIndex() == 2) {
        lfpratiometer.window_hamming();
        window_tracker_dummy = 2;
      }

      // clearing time series
      lfpratiometer.clrTimeSeries();

      break;

    case UNPAUSE:
      break;

    case PAUSE:
      break;

    case PERIOD:
      period = ((double)RT::System::getInstance()->getPeriod()) * 1e-9; // s
      break;

    default:
      break;
  }
}

// RTXI's customizeGUI function
void rtxilfpRatiometer::customizeGUI(void)
{
  QGridLayout* customlayout = DefaultGUIModel::getLayout();

  // adding dropdown menu for choosing FFT window shape
  windowShape = new QComboBox;
  windowShape->insertItem(1, "Rectangular");
  windowShape->insertItem(2, "Hamming");

  customlayout->addWidget(windowShape, 2, 0);
  setLayout(customlayout);
}