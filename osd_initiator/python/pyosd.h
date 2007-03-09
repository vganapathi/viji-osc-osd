/*
 * Declarations of objects found in all the source files.
 */
/*
 * OSDAttr type.  Wrapper around one attribute_list entry.
 */
struct pyosd_attr {
	PyObject_HEAD;
	struct attribute_list attr;
};

/*
 * OSDCommand type.
 */
struct pyosd_command {
	PyObject_HEAD;
	struct osd_command command;
	int set;
	int complete;
};

extern PyTypeObject pyosd_command_type;
extern PyTypeObject pyosd_attr_type;
extern PyTypeObject pyosd_device_type;
extern PyTypeObject pyosd_drive_type;
extern PyTypeObject pyosd_drivelist_type;

/* exporting this to python */
PyMODINIT_FUNC initpyosd(void);

