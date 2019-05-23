#!/usr/bin/env python3

import sys
import os
import datetime

#-----------------------------
# CXXCodeGen
#-----------------------------
def CXXCodeGen(class_name, class_types):
	'''Generate C++ code defining the class with class_name that inherits from JObject and has members given by class_types

	The return value is a list with the first item being string containing the C++ code defining the class.
	This should go into the header file. The second item is the code generated that defines the python
	bindings for the class using the syntax of the pybind11 package. This should go into the module pindings
	file in the section that defines the module.

	This is called from ProcessPyFile.
	'''

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

	cdef = ''
	cdef += '#ifndef _' + class_name + '_H_\n'
	cdef += '#define _' + class_name + '_H_\n'
	cdef += ''#include <JANA/JObject.h>\n'
	cdef += 'class '+class_name+' : public JObject\n'
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
	cdef += '};\n\n'
	cdef += '#endif // _' + class_name + '_H_\n'

	# Generate code for binding C++ class to Python class using pybind11
	pybindings = '\n'
	pybindings += '// ' + class_name + '\n'
	pybindings += 'py::class_<'+class_name+'>(m, "'+class_name+'")\n'
	pybindings += '.def(py::init<py::object&>())\n'
	for m,t in my_members.items():
		pybindings += '.def_readonly("'+m+'", &'+class_name+'::'+m+')\n'

	return (cdef, pybindings)


#-----------------------------
# ProcessPyFile
#-----------------------------
def ProcessPyFile(pyfname):
	'''Import the specified python file and return list of class definitions.

	The return value is a list of dictionaries with each dictionary representing
	a class. Dictionaries will contain the following entries:

	        name : class name
	       cdefs : string with contents to be written into C++ header file for class
	  pybindings : string with C++ code that should be added to the PYBIND11_MODULE section of the bindings file

	If there is a problem importing the specified python file then a error message
	is printed and the script exits.
	'''

	# Import the python source file containing one or more class definitions
	try:
		# Get list of all defined things before import so we can compare it
		# after import to see what was added
		module_name = pyfname[:-3]
		#pre_dir = dir()
		print('Reading JObject class from ' + pyfname)
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
	classdefs = []
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

		(cdef, pybindings) = CXXCodeGen( c, class_types)
		classdef = {'name':c, 'cdef':cdef, 'pybindings':pybindings, 'source_file':pyfname}
		classdefs.append(classdef)

	# Return lists of cdefs and pydefs for this file
	return classdefs


#-----------------------------
# MakeCXXHeader
#-----------------------------
def MakeCXXHeader(classdef):
	'''Make a C++ header file from the given classdef object.

	The name of the file will be based on the "name" entry of the classdef dictionary
	and the contents from the "cdefs" entry. Some additional header comments are added
	here.
	'''
	fname = classdef['name'] + '.h'
	with open( fname, 'w') as ofile:
		print( 'Creating ' + fname )
		ofile.write('''//
// ---------------  DO NOT EDIT - AUTOGENERATED FILE --------------
// (Please modify the original python file and regenerate if needed.)
//
// Generated from: ''' + classdef['source_file'] + '''
//           date: ''' + str(datetime.datetime.now()) + '''
//

''' + classdef['cdef'])

#-----------------------------
# PreambleBindingsFile
#-----------------------------
def PreambleBindingsFile( module_name, classdefs ):
	'''Return the preamble to write to the top of the bindings file.

	This is mainly put into its own routine for aesthetics of the global-scope code.
	'''

	preamble = '''
//--------------------  DO NOT EDIT - AUTOGENERATED FILE -------------------------
//================================================================================
// Python Bindings File.
//
// Generated: ''' + str(datetime.datetime.now()) + '''
//
// This auto-generated file contains the bindings for the members of classes
// accessible via Python. This was generated using one or more Python files as
// input to define the classes. At the same time this file was generated,
// additional C++ header files were generated that define the classes in C++.
// These C++ headers are included here:\n\n'''

	for classdef in classdefs:
		preamble += '#include "' + classdef['name'] + '.h"\n'

	preamble += '''
//-----------------------------------------------------------------------------
// Module definition
// The arguments of this structure tell Python what to call the extension,
// what it's members are and where to look for it's method definitions.
// The module name is taken from the directory name where the generator
// script was run from.

PYBIND11_MODULE(''' + module_name + ''', m) {\n'''

	return preamble

#==================================================================================
#                             main
#- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

GENERATECXXHEADERS = False



module_name = os.path.basename(os.getcwd())
all_classdefs  = []
for arg in sys.argv[1:]:
	if arg == '-h' : GENERATECXXHEADERS = True
	if arg.endswith('.py'):
		classdefs = ProcessPyFile( arg )
		if len(classdefs)>0 : all_classdefs.extend( classdefs )


# Create bindings file
fname_bindings = 'pybindings_' + module_name +'.cc'
print( 'Creating ' + fname_bindings )
with open( fname_bindings, 'w') as bfile:
	# Print header to bindings file
	bfile.write( PreambleBindingsFile(module_name, all_classdefs) )

	# Loop over classes
	for classdef in all_classdefs:

		# Write bindings source
		bfile.write( classdef['pybindings'] )

		# Create C++ header
		if GENERATECXXHEADERS : MakeCXXHeader( classdef )

	# Close bindings file
	bfile.write('\n}\n\n')
	bfile.close()

# Change back to working directory
#os.chdir( '..' )
