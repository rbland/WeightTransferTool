
#include <weightTransfer.h>

namespace WeightTransferTool
{
	// Checks for and returns the next valid shape
	// node dag path in the selection list.
	MDagPath get_shape_node(MItSelectionList& iter)
	{
		MDagPath dag_path;
		MObject component;
		MObject node;
		unsigned int num_shapes;

		if(iter.isDone())
		{
			display_error("Not enough objects selected.");
			// return an empty dag  path
			return MDagPath();
		}
		// typically the transform will be selected not the shape
		// node so extend the DAG path to the first shape node.
		iter.getDagPath(dag_path, component);
		dag_path.numberOfShapesDirectlyBelow(num_shapes);
		if(num_shapes > 0)
			dag_path.extendToShapeDirectlyBelow(0);
		// get shape node from dag path
		node = dag_path.node();
		if(!node.hasFn(MFn::kMesh))
		{
			display_error(MString("Node is not a mesh object: ") + dag_path.fullPathName());
			return MDagPath();
		}
		return dag_path;
	}

	// WeightTransfer creator function required by Maya plug-in.
	void* WeightTransfer::creator()
	{
		return new WeightTransfer();
	}

	// Main entry function to execute weight transfer command.
	MStatus WeightTransfer::doIt( const MArgList& args )
	{
		MStatus stat;
		if(args.length() < 2)
		{
			display_error("The weightTransfer command requires two arguments, a source and destination attribute.");
			return MS::kFailure;
		}

		MString source_attr_name;
		MString dest_attr_name;
		args.get(0, source_attr_name);
		args.get(1, dest_attr_name);

		MSelectionList selected;
		stat = MGlobal::getActiveSelectionList(selected);
		MCHECK_ERROR(stat);
		MItSelectionList iter( selected );

		// first selection is the source mesh
		MDagPath source_dag = get_shape_node(iter);
		if(!source_dag.isValid())
			return MS::kFailure;
		WeightsSource source(source_dag, source_attr_name);
		if(!source.is_valid)
			return MS::kFailure;

		// second selection is the destination mesh
		iter.next();
		MDagPath dest_dag = get_shape_node(iter);
		if(!dest_dag.isValid())
			return MS::kFailure;

		WeightsDestination dest(dest_dag, dest_attr_name);
		if(!dest.is_valid)
			return MS::kFailure;

		stat = dest.transfer_weights(source);
		if(stat)
			display_msg("Weights transferred succesfully!");
		return stat;
	}

	// WeightsSource class constructor.
	WeightsSource::WeightsSource(MDagPath& mesh_dag, MString weight_attr_name)
	{
		MStatus mesh_status = set_mesh(mesh_dag);
		MStatus attr_status = set_weight_attribute(weight_attr_name);
		
		retrieve_weights();
		
		// validate source mesh
		char buffer[MAX_STRING_SIZE];
		MString out_msg;
		if(vertex_count == 0)
		{
			display_error("The source mesh has zero vertices!");
			return;
		}
		if(vertex_count != weight_count)
		{
			sprintf_s(buffer, MAX_STRING_SIZE, "The source mesh's vertex count %d does not match the weight count %d.",
					   vertex_count, weight_count);
			display_error(buffer);
			return;
		}
		if(!mesh_status)
		{
			display_error("The source mesh was invalid.");
			return;
		}
		if(!attr_status)
		{
			display_error("The specified weight attribute is invalid.");
			return;
		}
		is_valid = true;

		MStatus stat;
		MObject mesh_obj = mesh_dag.node();

		// Construct Maya's mesh intersector which finds the closest
		// point on source mesh to a sample position.
		xform_matrix = mesh_dag.inclusiveMatrix();
		stat = intersector.create(mesh_obj, xform_matrix);
		MCHECK_ERROR(stat);
		
		unsigned poly_count = fn_mesh.numPolygons();

		weighted_polys = new WeightedPolygon[poly_count];
		weighted_verts = new WeightedVertex[vertex_count];

		MItMeshVertex vtx_iter(mesh_dag, MObject::kNullObj, &stat);
		MCHECK_ERROR(stat);

		MPoint position;
		double* weights;
		int index = 0;
		MIntArray connected_polys;
		MIntArray connected_edges;

		for( ; !vtx_iter.isDone(); vtx_iter.next())
		{
			position = vtx_iter.position(MSpace::kWorld, &stat);
			MCHECK_ERROR(stat);
			weights = get_weight(index);

			WeightedVertex& cur_vert = weighted_verts[index];
			cur_vert.set_vertex(position, weights);
			index++;
		}

		// initialize triangulated mesh data
		MIntArray tri_counts;
		MIntArray tri_verts;
		fn_mesh.getTriangles(tri_counts, tri_verts);

		unsigned start_index = 0;
		for(unsigned i=0; i < poly_count; i++)
		{
			weighted_polys[i].update_triangles(i, tri_counts[i], start_index,
												tri_verts, weighted_verts);
			start_index += tri_counts[i] * 3;
		}
	}

	// Samples the weight source mesh at an arbitray position in space.
	void WeightsSource::sample_mesh(const MPoint& sample_point, double* out_weights)
	{
		MPointOnMesh point_info;
		MPoint point(sample_point);
		intersector.getClosestPoint(point, point_info);
		int face_index = point_info.faceIndex();
		MPoint closest_pos = MPoint(point_info.getPoint());
		// the point returned by the mesh intersector is in the mesh's
		// local space but our data structures are in world space
		closest_pos *= xform_matrix;

		WeightedPolygon& poly = weighted_polys[face_index];
		WeightedVertex* matching_vert = poly.get_matching_vertex(closest_pos);
		if(matching_vert != NULL)
		{
			matching_vert->copy_weights(out_weights);
			return;
		}

		WeightedTriangle* tri = poly.get_intersected_triangle(closest_pos);
		tri->sample_weights(closest_pos, out_weights);
	}

	// WeightsDestination class constructor.
	WeightsDestination::WeightsDestination(MDagPath& mesh_dag, MString weight_attr_name)
	{
		MStatus mesh_stat = set_mesh(mesh_dag);
		MStatus weight_attr_stat = set_weight_attribute(weight_attr_name);

		if(mesh_stat && weight_attr_stat)
			is_valid = true;
	}

	// Transfers weights from the specified source to this mesh.
	MStatus WeightsDestination::transfer_weights(WeightsSource& source)
	{
		switch(weight_attr_type)
		{
			case MFnData::kDoubleArray:
				weight_double_vals.setLength(vertex_count);
				break;
			case MFnData::kVectorArray:
				weight_vector_vals.setLength(vertex_count);
				break;
			case MFnData::kPointArray:
				weight_point_vals.setLength(vertex_count);
				break;
			default:
				return MS::kFailure;
		}

		MStatus stat;
		MItMeshVertex vtx_iter(mesh_dag, MObject::kNullObj, &stat);
		double* weights = new double[4];
		unsigned index = 0;

		for(; !vtx_iter.isDone(&stat); vtx_iter.next())
		{
			MPoint p = vtx_iter.position(MSpace::kWorld, &stat);
			// Sample the source mesh for the current
			// point position and store the result.
			source.sample_mesh(p, weights);
			set_weight(index, weights);
			index++;
		}

		// assign weight values from array to weights attribute
		assign_weights();

		return MS::kSuccess;
	}
}
