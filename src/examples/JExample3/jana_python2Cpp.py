#!/usr/bin/env python3

import sys
import os

#-----------------------------
# MakeCXXClass
#-----------------------------
def MakeCXXClass(class_name, class_types):

	# Create dictionary that is subset of class members that
	# are only known types, replacing type with C++ type
	my_members = {}
	for m,t in class_types.items():
		st = str(t)
		if   st == "<class 'int'>":
			my_members[m] = 'int'
		elif st == "<class 'float'>":
			my_members[m] = 'float'
		elif st == "<class 'function'>":
			print('Methods are not supported. Sorry. (' + m + ' ignored)')
		else:
			print('Unknown or unsupported type '+str(t)+' for class ' + class_name + ' will be ignored')

	# Create some compact lists of member initializers/definitions
	# (do this here makes the code below easier to read)
	init_list = [] # list of member initializers for constructor e.g. ['a(a)', 'b(b)']
	for m,t in my_members.items(): init_list.append( '%s(%s)' % (m,m))
	mem_list = [] # list of member declarations e.g. ['int a', 'float b']
	for m,t in my_members.items(): mem_list.append( t + ' ' + m )

	cdef  = 'class '+class_name+' : public JObject\n'
	cdef += '{\n'
	cdef += '	public:\n'
	cdef += '		// Constructors\n'
	cdef += '		' + class_name + '(){}\n'
	cdef += '		' + class_name + '('
	cdef += ','.join(mem_list)
	cdef += ') : '
	cdef += ','.join(init_list)
	cdef += '{}\n'
	cdef += '\n'
	cdef += '		// Members\n'
	for d in mem_list: cdef += '		' +  d + ';\n'
	cdef += '};'

	# Generate code for binding C++ class to Python class using pybind11
	pybindings  = '// ' + class_name + '\n'
	pybindings += 'py::class_<'+class_name+'>(m, "'+class_name+'")\n'
	pybindings += '.def(py::init<py::object&>())\n'
	for m,t in my_members.items():
		pybindings += '.def_readonly("'+m+'", &'+class_name+'::'+m+')\n'

	return (cdef, pybindings)


#-----------------------------
# ProcessPyFile
#-----------------------------
def ProcessPyFile(pyfname):
	''' Make a C++ header file defining a class based on JObject that maps to the python class in the given input file.'''

	# Import the python source file containing one or more class definitions
	try:
		# Get list of all defined things before import so we can compare it
		# after import to see what was added
		module_name = pyfname[:-3]
		#pre_dir = dir()
		print('Making C++ JObject class from ' + pyfname)
		exec('import ' + module_name)
		_locals = locals()
		exec('global mod_items; mod_items = dir(%s)' % module_name)
		classes = [x for x in mod_items if not x.startswith('__')]
	except Exception as e:
		print('xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx')
		print('ERROR importing file: ' + pyfname)
		print('-----------------------------------------')
		print(e)
		print('-----------------------------------------')
		print('Please fix the error so python can import')
		print('the file cleanly and try again.          ')
		print('xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx')
		sys.exit(-1)

	# Loop over class names defined by input file
	cdefs  = []
	pydefs = []
	for c in classes:
		# Get members and types
		# We do this using the python "dir()" function, but it requires a non-string
		# argument so we must use exec and store the results in global variables.
		# We first fill the class_members global list of just member names and then
		# loop over those to fill the class_types global dictionary with keys being
		# member names and types being the result of python type().
		global class_members, class_types
		class_members = []
		class_types ={}
		exec('global class_members; class_members = dir(%s.%s)' % (module_name, c))
		class_members = [x for x in class_members if not x.startswith('__')]
		for m in class_members: exec('global class_types; class_types["%s"] = type(%s.%s.%s)' % (m, module_name, c, m))
		(cdef, pydef) = MakeCXXClass( c, class_types)
		cdefs.append(cdef)
		pydefs.append(pydef)

	# Return lists of cdefs and pydefs for this file
	return (cdefs, pydefs)



#==================================================================================

module_name = os.path.basename('.')
all_cdefs  = []
all_pydefs = []
for arg in sys.argv[1:]:
	if arg.endswith('.py'):
		(cdefs, pydefs) = ProcessPyFile( arg )
		all_cdefs.extend( cdefs )
		all_pydefs.extend( pydefs )

# Make directory to hold generated code to build plugin with bindings
if not os.path.exists( module_name ) : os.mkdir( module_name )

# Make C++ header file for each class
for cdef  in all_cdefs  :
	with of  = open( module_name +  )
	print( cdef + '\n'  )

	# Print python bindings
	print('''
	//================================================================================
	// Module definition
	// The arguments of this structure tell Python what to call your extension,
	// what it's methods are and where to look for it's method definitions
	PYBIND11_MODULE(''' + module_name + ''', m) {\n''')

	for pydef in pydefs : print( pydef )

	print('}\n\n')
