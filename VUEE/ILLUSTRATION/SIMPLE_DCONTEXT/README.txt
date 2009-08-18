This example illustrates an approach in which data specification is included
in one file (instead of separate data and data_init). We identify three
"data contexts" relevant from the viewpoint of PicOS <-> VUEE compatibility:

	- definition (DEF), i.e., the allocation of storage for the data
	- declaration (DCL), i.e., making the data visible in a given place
	- initialization (INI), i.e., assigning initial values to the data

This approach postulates a single ..._data_h file per praxis with three views
encapsulated into #ifdefs. It is more general and flexible from the viewpoint
of having multiple (cross-referencing) files per praxis.
