
struct VMFunction {};
struct VMObject {};

enum VMType {
	VM_NULL, VM_INT, VM_BOOL, VM_FLOAT, VM_STRING, VM_FUNCTION, VM_OBJECT
};

// Tagged Value
struct Value
{
	VMType type;
	union {
		int i;
		bool b;
		float f;
		const char* str;
		VMFunction func;
		VMObject obj;
	};
};

