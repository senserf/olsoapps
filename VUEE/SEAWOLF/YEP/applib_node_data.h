#ifndef __applib_node_data_h__
#define	__applib_node_data_h__

lcdg_dm_men_t 	*_da (lcd_menu);
nbh_menu_t	_da (nbh_menu);
rf_rcv_t	_da (rf_rcv), _da (ad_rcv);
word		_da (top_flag);
char 		*_da (ad_buf);
sea_rec_t	*_da (curr_rec);

#ifdef	__SMURPH__
lword		_da (host_id);
#endif

#endif
