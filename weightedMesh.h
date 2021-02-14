
#ifndef __WEIGHTED_MESH__
#define __WEIGHTED_MESH__

#include <weightTransferCommon.h>

namespace WeightTransferTool
{
	// a small number for comparing double vales
	const double EPSILON = 1E-5;

	// a two dimensional point position
	struct Point2d
	{
		double x;
		double y;
	};

	// enumeration to indicate the
    // largest axis of a vector
	enum MajorAxis
	{
		X_AXIS = 1,
		Y_AXIS = 2,
		Z_AXIS = 3,
	};

	// Tests two 2D points to see if the line segment they define crosses the positive X-axis.
	bool edge_crosses_x_axis(const Point2d&, const Point2d&);
	// Returns true if the number is greater than or equal to zero, false otherwise.
	bool simple_sign(double number);
	// Appends a value to an integer array if it is not already present.
	bool append_if_unique(MIntArray&, int);

	class WeightedVertex
	{
		public:
			WeightedVertex(){};						// WeightedVertex class constructor.
			~WeightedVertex(){};					// WeightedVertex class deconstructor.
			
			MPoint position;						// the vertex's position in world space
			double* weights;						// an array of up to 4 weights

			bool equals_position(const MPoint&);	// indicates if a sample position is equal to the vertex position
			void copy_weights(double*);				// returns a copy of this vertex's weights
			void set_vertex(MPoint&, double*);		// assigns the vertex position and weight
	};

	class WeightedTriangle
	{
		public:
			WeightedTriangle();						// WeightedTriangle class constructor.
			~WeightedTriangle();					// WeightedTriangle class deconstructor.
			void set_vertices(WeightedVertex*,
							  WeightedVertex*,
							  WeightedVertex*);		// Set the three vertices that make up this triangle and
													// store relevant triangle information.
			void sample_weights(const MPoint&,		// Calculates and returns the averaged weights of this
								double*) const;		// triangle at the specified sample position.
															
			bool point_is_inside(const MPoint&) const;		// Performs a fast test of the sample point to see if
															// it is inside this triangle.
			bool point_is_on_plane(const MPoint&) const;	// Tests the sample point to see if it lies in the plane of the triangle.
			bool point_is_inside_bary(const MPoint&) const; // Tests the sample point to see if it is inside this
															// triangle using barycentric coordinates.

		private:
			MVector get_bary_coords(const MVector&,	// Calculates the barycentric coordinates
									bool) const;	// of the sample point in this triangle.
			Point2d project_to_2d(const MPoint&) const;	// Projects a 3D point into 2D by removing a vector component.

			WeightedVertex* v0;						// The first weighted vertex of the triangle.
			WeightedVertex* v1;						// The second weighted vertex of the triangle.
			WeightedVertex* v2;						// The third weighted vertex of the triangle.

			MPoint centroid;						// The triangle centroid position.
			MVector normal;							// The triangle normal direction.
			MajorAxis major_axis;					// The major axis of the triangle. (i.e. its facing direction)
			double area_times_2;					// Two times the area of the triangle.

			Point2d v0_2d;							// The first vertex position in 2D.
			Point2d v1_2d;							// The second vertex position in 2D.
			Point2d v2_2d;							// The third vertex position in 2D.
	};

	class WeightedPolygon
	{
		public:
			WeightedPolygon();						// WeightedPolygon class constructor.
			~WeightedPolygon();						// WeightedPolygon class deconstructor.
			void update_triangles(unsigned, unsigned, unsigned,
								  const MIntArray&, WeightedVertex*);	// Updates the list of triangles that compose this weighted polygon.
			WeightedVertex* get_matching_vertex(const MPoint&);			// Tests this polygon's vertices to see if any have an equal position to the sample point.
			WeightedTriangle* get_intersected_triangle(const MPoint&);	// Find this polygon's triangle that contains the sample point.

		private:
			unsigned vertex_count;					// The number vertcies in this polygon.
			unsigned face_index;					// The index of this polygon in the parent mesh face array.
			unsigned triangle_count;				// The number of triangles in this polygon.
			WeightedTriangle* tris;					// The array of triangles which compose this polygon.
			WeightedVertex** verts;					// The array of vertices which compose this triangle.
	};

	// this class represents and manages a
	// poly mesh with vertex weights.
	class WeightedMesh
	{
		public:
			WeightedMesh();							// WeightedMesh class constructor.
			~WeightedMesh(){};						// WeightedMesh class deconstructor.
			MStatus set_mesh(MDagPath&);			// Sets this instance's source Maya mesh node.
			MStatus set_weight_attribute(MString);	// Sets the mesh node attribute name to find weight values in.
			double* get_weight(unsigned);			// Retrieves weight values for the specified vertex index.
			void set_weight(unsigned, const double*);// Sets weight values for the specified vertex index.
			bool is_valid;							

		protected:
			void retrieve_weights();				// Retrieves and stores weight information from the current weight attribute.
			void assign_weights();					// Assigns values stored in the internal weight arrays to the mesh's weight attribute.

			MFnMesh fn_mesh;						// The Maya mesh function object.
			unsigned vertex_count;					// The number of vertices in this mesh.
			unsigned weight_count;					// The number of weights in the weight array attribute.

			MDagPath mesh_dag;						// The DAG path to the Maya mesh node.
			MString attr_name;						// The weight attribute name.
			MPlug weight_plug;						// The weight data plug.
			MFnData::Type weight_attr_type;			// The weight attribute type enumerator.

			MFnDoubleArrayData plug_ddata;			// The weight data object for a doubleArray attribute.
			MFnVectorArrayData plug_vdata;			// The weight data object for a vectorArray attribute.
			MFnPointArrayData plug_pdata;			// The weight data object for a pointArray attribute.

			MDoubleArray weight_double_vals;		// The weights array for a doubleArray attribute.
			MVectorArray weight_vector_vals;		// The weights array for a vectorArray attribute.
			MPointArray weight_point_vals;			// The weights array for a pointArray attribute.
	};
}

#endif // end if undefined __WEIGHT_TRANSFER__