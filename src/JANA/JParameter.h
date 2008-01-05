// $Id: JParameter.h 1388 2005-12-13 14:13:24Z davidl $
//
//    File: JParameter.h
// Created: Fri Aug 12 12:36:42 EDT 2005
// Creator: davidl (on Darwin wire129.jlab.org 7.8.0 powerpc)
//

#ifndef _JParameter_
#define _JParameter_


#include <string>
#include <stdlib.h>
using namespace std;

#include "jerror.h"

class JILStream;

/// The JParameter class is used to hold one configuration parameter.
/// The objects are created and maintained internally via the JANA
/// framework. Users access and manipulate configuration parameters
/// via the JParameterManager class.

class JParameter{

	friend class JParameterManager;

	public:
		JParameter(string my_key, string my_value);
		JParameter(){}
		virtual ~JParameter(){}
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JParameter";}
		
		inline void SetValue(string val){value = val;}
		inline const string& GetKey(void){return key;}					///< Return key as a STL string
		inline const string& GetValue(void){return value;}				///< Return value as a STL string
		inline float f(void){return (float)atof(value.c_str());}		///< Return value as a float
		inline double d(void){return (double)atof(value.c_str());}	///< Return value as a double
		inline int i(void){return (int)atoi(value.c_str());}			///< Return value as an int
		void Dump(void);
		
		enum dataType_t{
			UNKNOWN,
			BOOL,
			CHAR,
			CHAR_PTR,
			CONST_CHAR_PTR,
			STRING,
			SHORT,
			INT,
			LONG,
			LONGLONG,
			UCHAR,
			USHORT,
			UINT,
			ULONG,
			ULONGLONG,
			FLOAT,
			DOUBLE
		};
		static inline dataType_t DataType(bool &v){return BOOL;}
		static inline dataType_t DataType(char &v){return CHAR;}
		static inline dataType_t DataType(char* &v){return CHAR_PTR;}
		static inline dataType_t DataType(const char* &v){return CONST_CHAR_PTR;}
		static inline dataType_t DataType(string &v){return STRING;}
		static inline dataType_t DataType(short &v){return SHORT;}
		static inline dataType_t DataType(int &v){return INT;}
		static inline dataType_t DataType(long &v){return LONG;}
		static inline dataType_t DataType(long long &v){return LONGLONG;}
		static inline dataType_t DataType(unsigned char &v){return UCHAR;}
		static inline dataType_t DataType(unsigned short &v){return USHORT;}
		static inline dataType_t DataType(unsigned int &v){return UINT;}
		static inline dataType_t DataType(unsigned long &v){return ULONG;}
		static inline dataType_t DataType(unsigned long long &v){return ULONGLONG;}
		static inline dataType_t DataType(float &v){return FLOAT;}
		static inline dataType_t DataType(double &v){return DOUBLE;}
		
		static const char* DataName(dataType_t type);
				
	protected:
		string key;
		string value;
		bool isdefault;	///< is the current value set by SetDefaultParameter ?
		bool hasdefault;	///< was SetDefaultParameter ever called for this key ?
		bool printme;		///< used by ParameterManager::PrintParameters()
		dataType_t type;	///< data type used in last set
	private:

};

#endif // _JParameter_

