#include <cstdio>
using namespace std;

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "rr_graph.h"
#include "rr_graph_util.h"
#include "rr_graph2.h"
#include "rr_graph_timing_params.h"

/****************** Subroutine definitions *********************************/

void add_rr_graph_C_from_switches(float C_ipin_cblock) {

	/* This routine finishes loading the C elements of the rr_graph. It assumes *
	 * that when you call it the CHANX and CHANY nodes have had their C set to  *
	 * their metal capacitance, and everything else has C set to 0.  The graph  *
	 * connectivity (edges, switch types etc.) must all be loaded too.  This    *
	 * routine will add in the capacitance on the CHANX and CHANY nodes due to: *
	 *                                                                          *
	 * 1) The output capacitance of the switches coming from OPINs;             *
	 * 2) The input and output capacitance of the switches between the various  *
	 *    wiring (CHANX and CHANY) segments; and                                *
	 * 3) The input capacitance of the input connection block (or buffers       *
	 *    separating tracks from the input connection block, if enabled by      *
	 *    INCLUDE_TRACK_BUFFERS)                                    	    */

	int inode, iedge, switch_index, to_node, maxlen;
	int icblock, isblock, iseg_low, iseg_high;
	float Cin, Cout;
	t_rr_type from_rr_type, to_rr_type;
	bool * cblock_counted; /* [0..max(g_ctx.nx,g_ctx.ny)] -- 0th element unused. */
	float *buffer_Cin; /* [0..max(g_ctx.nx,g_ctx.ny)] */
	bool buffered;
	float *Couts_to_add; /* UDSD */

	maxlen = max(g_ctx.nx, g_ctx.ny) + 1;
	cblock_counted = (bool *) vtr::calloc(maxlen, sizeof(bool));
	buffer_Cin = (float *) vtr::calloc(maxlen, sizeof(float));

	for (inode = 0; inode < g_ctx.num_rr_nodes; inode++) {

		from_rr_type = g_ctx.rr_nodes[inode].type();

		if (from_rr_type == CHANX || from_rr_type == CHANY) {

			for (iedge = 0; iedge < g_ctx.rr_nodes[inode].num_edges(); iedge++) {

				to_node = g_ctx.rr_nodes[inode].edge_sink_node(iedge);
				to_rr_type = g_ctx.rr_nodes[to_node].type();

				if (to_rr_type == CHANX || to_rr_type == CHANY) {

					switch_index = g_ctx.rr_nodes[inode].edge_switch(iedge);
					Cin = g_ctx.rr_switch_inf[switch_index].Cin;
					Cout = g_ctx.rr_switch_inf[switch_index].Cout;
					buffered = g_ctx.rr_switch_inf[switch_index].buffered;

					/* If both the switch from inode to to_node and the switch from *
					 * to_node back to inode use bidirectional switches (i.e. pass  *
					 * transistors), there will only be one physical switch for     *
					 * both edges.  Hence, I only want to count the capacitance of  *
					 * that switch for one of the two edges.  (Note:  if there is   *
					 * a pass transistor edge from x to y, I always build the graph *
					 * so that there is a corresponding edge using the same switch  *
					 * type from y to x.) So, I arbitrarily choose to add in the    *
					 * capacitance in that case of a pass transistor only when      *
					 * processing the lower inode number.                           *
					 * If an edge uses a buffer I always have to add in the output  *
					 * capacitance.  I assume that buffers are shared at the same   *
					 * (i,j) location, so only one input capacitance needs to be    *
					 * added for all the buffered switches at that location.  If    *
					 * the buffers at that location have different sizes, I use the *
					 * input capacitance of the largest one.                        */

					if (!buffered && inode < to_node) { /* Pass transistor. */
						g_ctx.rr_nodes[inode].set_C(g_ctx.rr_nodes[inode].C() + Cin);
						g_ctx.rr_nodes[to_node].set_C(g_ctx.rr_nodes[to_node].C() + Cout);
					}

					else if (buffered) {
						/* Prevent double counting of capacitance for UDSD */
						if (g_ctx.rr_nodes[to_node].direction() == BI_DIRECTION) {
							/* For multiple-driver architectures the output capacitance can
							 * be added now since each edge is actually a driver */
							g_ctx.rr_nodes[to_node].set_C(g_ctx.rr_nodes[to_node].C() + Cout);
						}
						isblock = seg_index_of_sblock(inode, to_node);
						buffer_Cin[isblock] = max(buffer_Cin[isblock], Cin);
					}

				}
				/* End edge to CHANX or CHANY node. */
				else if (to_rr_type == IPIN) {

					if (INCLUDE_TRACK_BUFFERS){
						/* Implements sharing of the track to connection box buffer.
						   Such a buffer exists at every segment of the wire at which
						   at least one logic block input connects. */
						icblock = seg_index_of_cblock(from_rr_type, to_node);
						if (cblock_counted[icblock] == false) {
							g_ctx.rr_nodes[inode].set_C(g_ctx.rr_nodes[inode].C() + C_ipin_cblock);
							cblock_counted[icblock] = true;
						}
					} else {
						/* No track buffer. Simply add the capacitance onto the wire */
						g_ctx.rr_nodes[inode].set_C(g_ctx.rr_nodes[inode].C() + C_ipin_cblock);
					}
				}
			} /* End loop over all edges of a node. */

			/* Reset the cblock_counted and buffer_Cin arrays, and add buf Cin. */

			/* Method below would be faster for very unpopulated segments, but I  *
			 * think it would be slower overall for most FPGAs, so commented out. */

			/*   for (iedge=0;iedge<g_ctx.rr_nodes[inode].num_edges();iedge++) {
			 * to_node = g_ctx.rr_nodes[inode].edges[iedge];
			 * if (g_ctx.rr_nodes[to_node].type() == IPIN) {
			 * icblock = seg_index_of_cblock (from_rr_type, to_node);
			 * cblock_counted[icblock] = false;
			 * }
			 * }     */

			if (from_rr_type == CHANX) {
				iseg_low = g_ctx.rr_nodes[inode].xlow();
				iseg_high = g_ctx.rr_nodes[inode].xhigh();
			} else { /* CHANY */
				iseg_low = g_ctx.rr_nodes[inode].ylow();
				iseg_high = g_ctx.rr_nodes[inode].yhigh();
			}

			for (icblock = iseg_low; icblock <= iseg_high; icblock++) {
				cblock_counted[icblock] = false;
			}

			for (isblock = iseg_low - 1; isblock <= iseg_high; isblock++) {
				g_ctx.rr_nodes[inode].set_C(g_ctx.rr_nodes[inode].C() + buffer_Cin[isblock]); /* Biggest buf Cin at loc */
				buffer_Cin[isblock] = 0.;
			}

		}
		/* End node is CHANX or CHANY */
		else if (from_rr_type == OPIN) {

			for (iedge = 0; iedge < g_ctx.rr_nodes[inode].num_edges(); iedge++) {
				switch_index = g_ctx.rr_nodes[inode].edge_switch(iedge);
				to_node = g_ctx.rr_nodes[inode].edge_sink_node(iedge);
				to_rr_type = g_ctx.rr_nodes[to_node].type();

				if (to_rr_type != CHANX && to_rr_type != CHANY)
					continue;

				if (g_ctx.rr_nodes[to_node].direction() == BI_DIRECTION) {
					Cout = g_ctx.rr_switch_inf[switch_index].Cout;
					to_node = g_ctx.rr_nodes[inode].edge_sink_node(iedge); /* Will be CHANX or CHANY */
					g_ctx.rr_nodes[to_node].set_C(g_ctx.rr_nodes[to_node].C() + Cout);
				}
			}
		}
		/* End node is OPIN. */
	} /* End for all nodes. */

	/* Now we need to add any Cout loads for nets that we previously didn't process
	 * Current structures only keep switch information from a node to the next node and
	 * not the reverse.  Therefore I need to go through all the possible edges to figure 
	 * out what the Cout's should be */
	Couts_to_add = (float *) vtr::calloc(g_ctx.num_rr_nodes, sizeof(float));
	for (inode = 0; inode < g_ctx.num_rr_nodes; inode++) {
		for (iedge = 0; iedge < g_ctx.rr_nodes[inode].num_edges(); iedge++) {
			switch_index = g_ctx.rr_nodes[inode].edge_switch(iedge);
			to_node = g_ctx.rr_nodes[inode].edge_sink_node(iedge);
			to_rr_type = g_ctx.rr_nodes[to_node].type();
			if (to_rr_type == CHANX || to_rr_type == CHANY) {
				if (g_ctx.rr_nodes[to_node].direction() != BI_DIRECTION) {
					/* Cout was not added in these cases */
					if (Couts_to_add[to_node] != 0) {
						/* We've already found a Cout to add to this node
						 * We could take the max of all possibilities but
						 * instead I will fail if there are conflicting Couts */
						if (Couts_to_add[to_node]
								!= g_ctx.rr_switch_inf[switch_index].Cout) {
							vpr_throw(VPR_ERROR_ROUTE, __FILE__,__LINE__,
								"A single driver resource (%i) is driven by different Cout's (%e!=%e)\n",
									to_node, Couts_to_add[to_node],
									g_ctx.rr_switch_inf[switch_index].Cout);
						}
					}
					Couts_to_add[to_node] = g_ctx.rr_switch_inf[switch_index].Cout;

				}
			}
		}
	}
	for (inode = 0; inode < g_ctx.num_rr_nodes; inode++) {
		g_ctx.rr_nodes[inode].set_C(g_ctx.rr_nodes[inode].C() + Couts_to_add[inode]);
	}
	free(Couts_to_add);
	free(cblock_counted);
	free(buffer_Cin);
}
