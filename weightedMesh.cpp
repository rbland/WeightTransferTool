
#include <weightedMesh.h>

namespace WeightTransferTool
{
	// Tests two 2D points to see if the line segment they define crosses the positive X-axis.
	bool edge_crosses_x_axis(const Point2d& p0, const Point2d& p1)
	{
		if(p0.y == 0.0 && p1.y == 0.0)
			// The edge is on the X-axis, count this as an intersection
			// as long as the segment is partially positive.
			return p0.x > 0 || p1.x > 0;
		if(simple_sign(p0.y) == simple_sign(p1.y))
			// Both end points are on the same side of the X-axis,
			// so the edge cannot cross it.
			return false;
		if(simple_sign(p0.x) && simple_sign(p1.x))
			// Both end points are to the right of the Y-axis but opposite 
			// sides of the X-axis. A positive intersection must occur.
			return true;
		if(!simple_sign(p0.x) && !simple_sign(p1.x))
			// Both end points are to the left of the Y-axis.
			// No intersection with the positive X-axis is possible.
			return false;

		// The edge crosses the X-axis.
		// Calculate the x-intercept.
		//
		double inv_slope = (p1.x - p0.x) / (p1.y - p0.y);
		double x_int = p0.x - inv_slope * p0.y;

		// check for positive value
		return simple_sign(x_int);
	}

	// Returns true if the number is greater than or equal to zero, false otherwise.
	bool simple_sign(double number)
	{
		return number >= 0;
	}

	// Appends a value to an integer array if it is not already present.
	bool append_if_unique(MIntArray& int_array, int new_value)
	{
		for(unsigned i = 0; i < int_array.length(); i++)
			if(int_array[i] == new_value)
				return false;
		int_array.append(new_value);
		return true;
	}

	// WeightedMesh class constructor.
	WeightedMesh::WeightedMesh()
	{
		attr_name = MString("");
		is_valid = false;
		vertex_count = 0;
		weight_count = 0;
	}

	// Sets this instance's source Maya mesh node.
	MStatus WeightedMesh::set_mesh(MDagPath& new_mesh_dag)
	{
		mesh_dag = new_mesh_dag;
		MStatus stat = fn_mesh.setObject(new_mesh_dag);
		MCHECK_ERROR(stat);
		vertex_count = fn_mesh.numVertices();
		return stat;
	}

	// Sets the mesh node attribute name to find weight values in.
	MStatus WeightedMesh::set_weight_attribute(MString weight_attr_name)
	{
		MStatus stat;
		weight_plug = fn_mesh.findPlug(weight_attr_name, true, &stat);
		
		if(!stat)
		{
			display_error(MString("Unable to find weight plug: ") + weight_attr_name);
			return MS::kFailure;
		}

		// get weight attribute information
		MObject weight_attr = weight_plug.attribute();
		if(!weight_attr.hasFn(MFn::kTypedAttribute))
		{
			display_error(MString("Invalid weights attribute: ") + weight_attr_name);
			return MS::kFailure;
		}
		MFnTypedAttribute fn_weight_attr(weight_attr);
		weight_attr_type = fn_weight_attr.attrType(&stat);
		MCHECK_ERROR(stat);
		
		switch(weight_attr_type)
		{
			// This class can read and write weights from a
			// double, vector or point array attribute type.
			case MFnData::kDoubleArray:
			case MFnData::kVectorArray:
			case MFnData::kPointArray:
				break;
			default:
				display_error(MString("The weight attribute type is not supported.  ") + 
							  MString("The attribute must be doubleArray, pointArray or vectorArray."));
				return MS::kFailure;
		}
		return MS::kSuccess;
	}

	// Retrieves weight values for the specified vertex index.
	double* WeightedMesh::get_weight(unsigned index)
	{
		double* weight_val = new double[4];
		
		double dvalue;
		MVector vvalue;
		MPoint pvalue;
		switch(weight_attr_type)
		{
			case MFnData::kDoubleArray:
				// handle double weights
				dvalue = weight_double_vals[index];
				weight_val[0] = dvalue;
				weight_val[1] = dvalue;
				weight_val[2] = dvalue;
				weight_val[3] = dvalue;
				break;
			case MFnData::kVectorArray:
				// handle vector 3 weights
				vvalue = weight_vector_vals[index];
				weight_val[0] = vvalue.x;
				weight_val[1] = vvalue.y;
				weight_val[2] = vvalue.z;
				weight_val[3] = 0.0;
				break;
			case MFnData::kPointArray:
				// handle vector 4 weights
				pvalue = weight_point_vals[index];
				weight_val[0] = pvalue.x;
				weight_val[1] = pvalue.y;
				weight_val[2] = pvalue.z;
				weight_val[3] = pvalue.w;
				break;
			default:
				break;
		}
		return weight_val;
	}

	// Sets weight values for the specified vertex index.
	void WeightedMesh::set_weight(unsigned index, const double* weights)
	{
		switch(weight_attr_type)
		{
			case MFnData::kDoubleArray:
				weight_double_vals.set(weights[0], index);
				break;
			case MFnData::kVectorArray:
				weight_vector_vals.set(MVector(weights[0], weights[1],
											   weights[2]), index);
				break;
			case MFnData::kPointArray:
				weight_point_vals.set(MPoint(weights[0], weights[1],
											 weights[2], weights[3]), index);
				break;
			default:
				break;
		}
	}

	// Retrieves and stores weight information from the current weight attribute.
	void WeightedMesh::retrieve_weights()
	{
		MObject plug_mobject = weight_plug.asMObject();
		weight_double_vals.clear();
		weight_vector_vals.clear();
		weight_point_vals.clear();

		switch(weight_attr_type)
		{
			case MFnData::kDoubleArray:
				// handle double weights
				plug_ddata.setObject(plug_mobject);
				weight_double_vals = plug_ddata.array();
				weight_count = weight_double_vals.length();
				break;
			case MFnData::kVectorArray:
				// handle vector 3 weights
				plug_vdata.setObject(plug_mobject);
				weight_vector_vals = plug_vdata.array();
				weight_count = weight_vector_vals.length();
				break;
			case MFnData::kPointArray:
				// handle vector 4 weights
				plug_pdata.setObject(plug_mobject);
				weight_point_vals = plug_pdata.array();
				weight_count = weight_point_vals.length();
				break;
			default:
				break;
		}
	}

	// Assigns values stored in the internal weight arrays to the mesh's weight attribute.
	void WeightedMesh::assign_weights()
	{
		MStatus stat;
		MObject weights_mobject;

		switch(weight_attr_type)
		{
			case MFnData::kDoubleArray:
				// handle double weights
				weights_mobject = plug_ddata.create(weight_double_vals, &stat);
				break;
			case MFnData::kVectorArray:
				// handle vector 3 weights
				weights_mobject = plug_vdata.create(weight_vector_vals, &stat);
				break;
			case MFnData::kPointArray:
				// handle vector 4 weights
				weights_mobject = plug_pdata.create(weight_point_vals, &stat);
				break;
			default:
				break;
		}
		weight_plug.setMObject(weights_mobject);
	}

	// WeightedPolygon class constructor.
	WeightedPolygon::WeightedPolygon()
	{
		triangle_count = 0;
		face_index = 0;
		tris = NULL;
	}

	// WeightedPolygon class deconstructor.
	WeightedPolygon::~WeightedPolygon()
	{
		tris = NULL;
		verts = NULL;
	}

	// Updates the list of triangles that compose this weighted polygon.
	void WeightedPolygon::update_triangles(unsigned my_face_index,
										   unsigned new_triangle_count,
										   unsigned start_index,
										   const MIntArray& tri_vert_indexes,
										   WeightedVertex* all_verts)
	{
		face_index = my_face_index;
		triangle_count = new_triangle_count;
		tris = new WeightedTriangle[new_triangle_count];
		WeightedVertex *v0, *v1, *v2;
		MIntArray uniqe_verts_indexes;
		// triangle vertex indexes
		unsigned i0, i1, i2;

		for(unsigned i=0; i < new_triangle_count; i++)
		{
			i0 = tri_vert_indexes[start_index];
			i1 = tri_vert_indexes[start_index+1];
			i2 = tri_vert_indexes[start_index+2];
			// keep an array of the unique indexes which make up this polygon
			append_if_unique(uniqe_verts_indexes, i0);
			append_if_unique(uniqe_verts_indexes, i1);
			append_if_unique(uniqe_verts_indexes, i2);
			// get vertices from array of all mesh verts
			v0 = &all_verts[i0];
			v1 = &all_verts[i1];
			v2 = &all_verts[i2];
			// update triangle data
			tris[i].set_vertices(v0, v1, v2);
			start_index += 3;
		}

		vertex_count = uniqe_verts_indexes.length();
		verts = new WeightedVertex*[vertex_count];
		for(unsigned i=0; i < vertex_count; i++)
			verts[i] = &all_verts[uniqe_verts_indexes[i]];
	}

	// Tests this polygon's vertices to see if any have an equal position to the sample point.
	WeightedVertex* WeightedPolygon::get_matching_vertex(const MPoint& sample_point)
	{
		for(unsigned i = 0; i < vertex_count; i++)
			if(verts[i]->equals_position(sample_point))
				return verts[i];
		return (WeightedVertex*)NULL;
	}

	// Find this polygon's triangle that contains the sample point.
	WeightedTriangle* WeightedPolygon::get_intersected_triangle(const MPoint& sample_point)
	{
		// Perfrom a simple test to detemine what triangle
		// contains the sample point.
		for(unsigned i = 0; i < triangle_count; i++)
			if(tris[i].point_is_inside(sample_point))
				return &tris[i];

		// If not triangle could be found do a more sophisticated
		// barycentric coordinate test to determine the triangle
		// which contains the point.
		for(unsigned i = 0; i < triangle_count; i++)
			if(tris[i].point_is_inside_bary(sample_point))
				return &tris[i];

		// This should never happen.
		display_error("No intersected triangle found!");
		// Return first triangle by default.
		return &tris[0];
	}

	// WeightedTriangle class constructor.
	WeightedTriangle::WeightedTriangle()
	{
		v0 = NULL;
		v1 = NULL;
		v2 = NULL;
		area_times_2 = 1;
	}

	// WeightedTriangle class deconstructor.
	WeightedTriangle::~WeightedTriangle()
	{
		v0 = NULL;
		v1 = NULL;
		v2 = NULL;
	}

	// Set the three vertices that make up this triangle and
	// store relevant triangle information.
	void WeightedTriangle::set_vertices(WeightedVertex* new_v0,
										WeightedVertex* new_v1,
										WeightedVertex* new_v2)
	{
		// store vertices as class attributes
		v0 = new_v0;
		v1 = new_v1;
		v2 = new_v2;

		MPoint p0 = v0->position;
		MPoint p1 = v1->position;
		MPoint p2 = v2->position;

		// calculate triangle centroid
		//
		centroid = MPoint(0.0, 0.0, 0.0);
		centroid += MVector(p0);
		centroid += MVector(p1);
		centroid += MVector(p2);
		centroid = centroid / 3.0;

		// calculate triangle normal and area
		//
		MVector e0 = p1 - p0;
		MVector e1 = p2 - p0;
		normal = e0 ^ e1;
		area_times_2 = normal.length();
		normal = normal / area_times_2;

		// The major axis is the largest absolute component of the triangle's
		// normal.  The major axis is the axis along which points on this triangle
		// will be project into 2D. This ensures we get the least distorted
		// 2D approximation and never project to a line.
		MVector abs_normal = MVector(abs(normal.x),
									 abs(normal.y),
									 abs(normal.z));
		if(abs_normal.x > abs_normal.y)
		{
			if(abs_normal.x > abs_normal.z)
				major_axis = X_AXIS;
			else if(abs_normal.y > abs_normal.z)
				major_axis = Y_AXIS;
			else
				major_axis = Z_AXIS;
		}
		else
		{
			if(abs_normal.y > abs_normal.z)
				major_axis = Y_AXIS;
			else
				major_axis = Z_AXIS;
		}

		// Define simplified triangle projected into 2D.
		v0_2d = project_to_2d(p0);
		v1_2d = project_to_2d(p1);
		v2_2d = project_to_2d(p2);
	}

	// Calculates and returns the weights of this triangle at the specified sample position.
	void WeightedTriangle::sample_weights(const MPoint& sample_point, double* out_weights) const
	{
		MVector bary_coords = get_bary_coords(sample_point, true);
		double w0, w1, w2;
		for(unsigned i = 0; i < 4; i++)
		{
			w0 = v0->weights[i] * bary_coords.x;
			w1 = v1->weights[i] * bary_coords.y;
			w2 = v2->weights[i] * bary_coords.z;
			out_weights[i] = w0 + w1 + w2;
		}
	}

	// Performs a fast test of the sample point to see if it is inside this triangle.
	bool WeightedTriangle::point_is_inside(const MPoint& sample_point) const
	{
		if(!point_is_on_plane(sample_point))
			return false;

		Point2d sample_2d = project_to_2d(sample_point);
		// adjust the 2D triangle so that the sample point is the origin.
		Point2d adj_v0 = {v0_2d.x - sample_2d.x, v0_2d.y - sample_2d.y};
		Point2d adj_v1 = {v1_2d.x - sample_2d.x, v1_2d.y - sample_2d.y};
		Point2d adj_v2 = {v2_2d.x - sample_2d.x, v2_2d.y - sample_2d.y};

		// Test to see how many adjusted triangle edges intersect the positive X-axis.
		unsigned intersections = 0;
		if(edge_crosses_x_axis(adj_v0, adj_v1))
			intersections++;
		if(edge_crosses_x_axis(adj_v1, adj_v2))
			intersections++;
		if(edge_crosses_x_axis(adj_v0, adj_v2))
			intersections++;

		// An odd number of intersection indicates the sample point is inside the triangle.
		return (intersections % 2 == 1);
	}
	
	// Tests the sample point to see if it lies in the plane of the triangle.
	bool WeightedTriangle::point_is_on_plane(const MPoint& sample_point) const
	{
		// direction from point on triangle to the sample position
		MVector sample_direction = sample_point - centroid;
		sample_direction.normalize();

		// dot product of sample direction and normal direction
		double cos_theta = sample_direction * normal;
		// A result close to zero indicates the sample
		// direction is orthogonal to the triangle noraml.
		return abs(cos_theta) < EPSILON;
	}

	// Tests the sample point to see if it is inside this triangle using barycentric coordinates.
	bool WeightedTriangle::point_is_inside_bary(const MPoint& sample_point) const
	{
		MVector bary_coords = get_bary_coords(sample_point, false);
		double total_area = bary_coords.x + bary_coords.y + bary_coords.z;
		return abs(1.0 - total_area) < EPSILON;
	}

	// Calculates the barycentric coordinates of the sample point in this triangle.
	MVector WeightedTriangle::get_bary_coords(const MVector& sample_point,
														  bool normalized) const
	{
		MVector out_bary_coords = MVector::xAxis;
		MVector cross;

		// Calculate areas of triangle fragmenets created
		// by sample point inside of large triangle.
		MVector e0 = v0->position - sample_point;
		MVector e1 = v1->position - sample_point;
		MVector e2 = v2->position - sample_point;
		
		// Each bary coordinate is defined as the fraction of the
		// larger area occupied by each triangle fragment.
		cross = e2 ^ e1;
		out_bary_coords.x = cross.length() / area_times_2;
		cross = e0 ^ e2;
		out_bary_coords.y = cross.length() / area_times_2;

		if(normalized)
		{
			// Calculating the final coordinate this ways is faster
			// and guarantees normalized coordinates that sum to 1.
			out_bary_coords.z = 1 - (out_bary_coords.y + out_bary_coords.x);
		}
		else
		{
			// Calculate the true coordinates which may sum to more than 1/
			cross = e0 ^ e1;
			out_bary_coords.z = cross.length() / area_times_2;
		}
		return out_bary_coords;
	}

	// Projects a 3D point into 2D by removing a vector component.
	Point2d WeightedTriangle::project_to_2d(const MPoint& position) const
	{
		Point2d pos_2d;
		switch(major_axis)
		{
			case X_AXIS:
				pos_2d.x = position.y;
				pos_2d.y = position.z;
				break;
			case Y_AXIS:
				pos_2d.x = position.x;
				pos_2d.y = position.z;
				break;
			case Z_AXIS:
				pos_2d.x = position.x;
				pos_2d.y = position.y;
				break;
		}
		return pos_2d;
	}

	// Tests the sample point to see if it equals vertex position.
	bool WeightedVertex::equals_position(const MPoint& sample_point)
	{
		MVector delta = sample_point - position;

		// Allow for a small error tolerance.
		return (abs(delta.x) < EPSILON &&
				abs(delta.y) < EPSILON &&
				abs(delta.z) < EPSILON);
	}

	// Gets a copy of this vertex's weights.
	void WeightedVertex::copy_weights(double* out_weights)
	{
		memcpy(out_weights, weights, sizeof(double) * 4);
	}

	// Sets this vertex's position and weights.
	void WeightedVertex::set_vertex(MPoint& new_position, double* new_weights)
	{
		position = new_position;
		weights = new_weights;
	}
} // end namespace WeightTransferTool