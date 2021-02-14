
#ifndef __WEIGHT_TRANSFER_COMMON__
#define __WEIGHT_TRANSFER_COMMON__

// required Maya includes
#include <maya/MTypes.h>
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MDagPath.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MItMeshVertex.h>
#include <maya/MDoubleArray.h>
#include <maya/MVectorArray.h>
#include <maya/MPointArray.h>
#include <maya/MMeshIntersector.h>

#include <maya/MFnMesh.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnData.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnPointArrayData.h>
#include <maya/MFnVectorArrayData.h>

// pre-processor functions.
#define MAX_STRING_SIZE 100
#define MATRIX_ROW "%0.3f %0.3f %0.3f %0.3f"

// Prints a string and double to the Maya console.
#define print_double(msg, value)	\
	MGlobal::displayInfo(MString(msg) + value);

// Prints a vector 2 to the Maya console.
#define print_vector2(msg, vec)	\
	sprintf_s(buffer, MAX_STRING_SIZE, "%0.3f, %0.3f", vec.x, vec.y);	\
	MGlobal::displayInfo(MString(msg) + buffer);

// Prints a vector 3 to the Maya console.
#define print_vector3(msg, vec)	\
	sprintf_s(buffer, MAX_STRING_SIZE, "%0.3f, %0.3f, %0.3f", vec.x, vec.y, vec.z);	\
	MGlobal::displayInfo(MString(msg) + buffer);

// Prints a matrix to the Maya console.
#define print_matrix(msg, mat)					\
	MGlobal::displayInfo(msg);					\
	for(unsigned _row = 0; _row < 4; _row++)	\
	{											\
		sprintf_s(buffer, MAX_STRING_SIZE, MATRIX_ROW,	\
					mat(_row,0), mat(_row,1), mat(_row,2), mat(_row,3));	\
		MGlobal::displayInfo(buffer);			\
	}

// Prints the MStatus object error if it is a failure.
#define MCHECK_ERROR(stat)\
	if(!stat)			 \
		stat.perror("");

// Prints a message to the Maya console.
#define display_msg(msg)\
	MGlobal::displayInfo(msg);

// Prints an error message to the Maya console.
#define display_error(msg)\
	MGlobal::displayError(msg);

// Frees a memory pointer and assigns NULL to it.
#define free_pointer(ptr)\
	if(ptr != NULL)	\
	{				\
		free(ptr);	\
		ptr = NULL;	\
	}

// Frees an array of pointers.
#define free_array(ptr, len)		\
	if(ptr != NULL)					\
	{								\
		for(unsigned ctr = 0; ctr < len; ctr++)	\
		{							\
			if(ptr[ctr] != NULL)	\
				free(ptr[ctr]);		\
			ptr[ctr] = NULL;		\
		}							\
		free(ptr);					\
		ptr = NULL;					\
	}

#endif // end if undefined __WEIGHT_TRANSFER_COMMON__