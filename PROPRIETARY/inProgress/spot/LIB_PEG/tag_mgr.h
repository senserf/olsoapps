#ifndef __tag_mgr_h
#define __tag_mgr_h

extern tagListType tagList;

void reset_tags ();
word del_tag (word id, word ref, word dupeq, Boolean force);
void ins_tag (char * buf, word rssi);
void report_tag (char * td);

//+++ tag_mgr.cc

#endif
