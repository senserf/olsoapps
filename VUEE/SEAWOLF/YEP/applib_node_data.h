#ifndef __applib_node_data_h__
#define	__applib_node_data_h__

lcdg_dm_men_t 	*lcd_menu;
nbh_menu_t	nbh_menu;
rf_rcv_t	rf_rcv, ad_rcv;
word		top_flag;
char 		*ad_buf;
sea_rec_t	*curr_rec;

#ifdef	__SMURPH__
lword		host_id;
#endif

#endif
