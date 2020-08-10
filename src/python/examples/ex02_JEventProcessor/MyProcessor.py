#
# This example shows how to create a subclass of JEventProcessor in python
# and use it to access data objects. The class definition is in the top part
# of the file and the bottom part configures and runs JANA so we can test it.
# Run this like:
#
#    python3 MyProcess.py
#
import jana

#===========================================================================
# Define class based on JEventProcessor
class MyProcessor( jana.JEventProcessor ):
	def __init__(self):
		super().__init__(self)

	#-------------------------------------------
	# Called once at program startup
	def Init( self ):
		print('Python Init called')

	#-------------------------------------------
	# Called every event
	def Process( self, aEvent ):
		# Get run number and event number (if you need them)
		runnumber = aEvent.GetRunNumber()
		eventnumber = aEvent.GetEventNumber()

		# Get JTestEventData data objects (these are just example objects the JTest plugin produces)
		eventdataobjs = aEvent.Get(jana.JTestEventData)

		# Print message. In a real application you would do something like fill histograms
		print('Run / event number: %d / %d   Ntest_objects=%d' % (runnumber, eventnumber, len(eventdataobjs)))

	#-------------------------------------------
	# Called once at program end
	def Finish( self ):
		print('Python Finish called')

#===========================================================================
# Configure and run JANA using our MyProcessor

if __name__ == "__main__":

	jana.SetParameterValue( 'JANA:DEBUG_PLUGIN_LOADING', '1')
	jana.SetParameterValue( 'NTHREADS', '1')
	jana.SetParameterValue( 'JANA:NEVENTS', '3')

	jana.AddProcessor( MyProcessor() )
	jana.AddPlugin('JTest')
	jana.Run()

	print('PYTHON DONE.')

