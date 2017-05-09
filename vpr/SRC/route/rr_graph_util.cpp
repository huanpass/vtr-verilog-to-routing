#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "rr_graph_util.h"

t_linked_edge *
insert_in_edge_list(t_linked_edge * head, const int edge, const short iswitch) {

	/* Inserts a new element at the head of a linked list.  Returns the new head *
	 * of the list.  One argument is the address of the head of a list of free   *
	 * edge_list elements.  If there are any elements on this free list, the new *
	 * element is taken from it.  Otherwise a new one is malloced.               */

	t_linked_edge *linked_edge;

	linked_edge = (t_linked_edge *) vtr::malloc(sizeof(t_linked_edge));

	linked_edge->edge = edge;
	linked_edge->iswitch = iswitch;
	linked_edge->next = head;

	return linked_edge;
}

#if 0
void
free_linked_edge_soft(INOUT t_linked_edge * edge_ptr,
		INOUT t_linked_edge ** free_list_head_ptr)
{

	/* This routine does a soft free of the structure pointed to by edge_ptr by *
	 * adding it to the free list.  You have to pass in the address of the      *
	 * head of the free list.                                                   */

	edge_ptr->next = *free_list_head_ptr;
	*free_list_head_ptr = edge_ptr;
}
#endif

int seg_index_of_cblock(t_rr_type from_rr_type, int to_node) {

	/* Returns the segment number (distance along the channel) of the connection *
	 * box from from_rr_type (CHANX or CHANY) to to_node (IPIN).                 */

	if (from_rr_type == CHANX)
		return (g_ctx.rr_nodes[to_node].xlow());
	else
		/* CHANY */
		return (g_ctx.rr_nodes[to_node].ylow());
}

int seg_index_of_sblock(int from_node, int to_node) {

	/* Returns the segment number (distance along the channel) of the switch box *
	 * box from from_node (CHANX or CHANY) to to_node (CHANX or CHANY).  The     *
	 * switch box on the left side of a CHANX segment at (i,j) has seg_index =   *
	 * i-1, while the switch box on the right side of that segment has seg_index *
	 * = i.  CHANY stuff works similarly.  Hence the range of values returned is *
	 * 0 to g_ctx.nx (if from_node is a CHANX) or 0 to g_ctx.ny (if from_node is a CHANY).   */

	t_rr_type from_rr_type, to_rr_type;

	from_rr_type = g_ctx.rr_nodes[from_node].type();
	to_rr_type = g_ctx.rr_nodes[to_node].type();

	if (from_rr_type == CHANX) {
		if (to_rr_type == CHANY) {
			return (g_ctx.rr_nodes[to_node].xlow());
		} else if (to_rr_type == CHANX) {
			if (g_ctx.rr_nodes[to_node].xlow() > g_ctx.rr_nodes[from_node].xlow()) { /* Going right */
				return (g_ctx.rr_nodes[from_node].xhigh());
			} else { /* Going left */
				return (g_ctx.rr_nodes[to_node].xhigh());
			}
		} else {
			vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
				"in seg_index_of_sblock: to_node %d is of type %d.\n",
					to_node, to_rr_type);
			return OPEN; //Should not reach here once thrown
		}
	}
	/* End from_rr_type is CHANX */
	else if (from_rr_type == CHANY) {
		if (to_rr_type == CHANX) {
			return (g_ctx.rr_nodes[to_node].ylow());
		} else if (to_rr_type == CHANY) {
			if (g_ctx.rr_nodes[to_node].ylow() > g_ctx.rr_nodes[from_node].ylow()) { /* Going up */
				return (g_ctx.rr_nodes[from_node].yhigh());
			} else { /* Going down */
				return (g_ctx.rr_nodes[to_node].yhigh());
			}
		} else {
			vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
				 "in seg_index_of_sblock: to_node %d is of type %d.\n",
					to_node, to_rr_type);
			return OPEN; //Should not reach here once thrown
		}
	}
	/* End from_rr_type is CHANY */
	else {
		vpr_throw(VPR_ERROR_ROUTE, __FILE__, __LINE__, 
			"in seg_index_of_sblock: from_node %d is of type %d.\n",
				from_node, from_rr_type);
		return OPEN; //Should not reach here once thrown
	}
}
