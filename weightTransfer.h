
#ifndef __WEIGHT_TRANSFER__
#define __WEIGHT_TRANSFER__

#include <stdlib.h>

#include <maya/MFnPlugin.h>
#include <maya/MPxCommand.h>
#include <maya/MArgList.h>

#include <weightedMesh.h>
#include <weightTransferCommon.h>

#define PLUGIN_NAME "weightTransfer"

namespace WeightTransferTool
{
	MDagPath get_shape_node(MItSelectionList&);			// Checks for and returns the next valid shape
														// node dag path in the selection list.

	// This class manages and samples
	// weight values from the source mesh
	class WeightsSource : public WeightedMesh
	{
		public:
			WeightsSource(MDagPath&, MString);			// WeightsSource class constructor.
			~WeightsSource(){};							// WeightsSource class deconstructor.

			// source weight sample methods
			void sample_mesh(const MPoint&, double*);	// Samples the weight source mesh at
							 							// an arbitray position in space.
			
		private:
			MMatrix xform_matrix;						// the world transform matrix for this mesh.
			WeightedPolygon* weighted_polys;			// The array of polygons that make up this mesh.
			WeightedVertex* weighted_verts;				// The array of all vertices that make up this mesh.
			MMeshIntersector intersector;				// The Maya mesh intersector which calculates the closest point on surface.
	};

	// this class applies weights from the
	// source mesh to a destination mesh
	class WeightsDestination : public WeightedMesh
	{
		public:
			WeightsDestination(MDagPath&, MString);		// WeightsDestination class constructor.
			~WeightsDestination(){};					// WeightsDestination class deconstructor.
			MStatus transfer_weights(WeightsSource&);	// Transfers weights from the specified source to this mesh.
	};

	// The main weight transfer command class parses the
	// input arguments and executes the weight transfer.
	class WeightTransfer : public MPxCommand
	{
		public:
			WeightTransfer(){};							// WeightTransfer class constructor.
			virtual ~WeightTransfer(){};				// WeightTransfer class deconstructor.

			virtual MStatus doIt ( const MArgList& args );	// plug-in entry function
			static void* creator();						// plug-in class instantiation function
	};

} // end namespace WeightTransferTool

// intialize weight transfer plug-in
MStatus initializePlugin( MObject obj )
{
	MFnPlugin plugin( obj, "rbland", "1.0.0", "Any");
	// register the weightTransfer plug-in command in Maya
	MStatus status = plugin.registerCommand(PLUGIN_NAME,
		WeightTransferTool::WeightTransfer::creator );
	MCHECK_ERROR(status);
	return status;
}

// unintialize weight transfer plug-in
MStatus uninitializePlugin( MObject obj )
{
	MFnPlugin plugin( obj );
	MStatus status = plugin.deregisterCommand(PLUGIN_NAME);
	MCHECK_ERROR(status);
	return status;
}

#endif // end if undefined __WEIGHT_TRANSFER__