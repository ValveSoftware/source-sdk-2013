//======= Copyright © 1996-2006, Valve Corporation, All rights reserved. ======
//
// Purpose: Utility classes for creating, registering & deregistering
//          Maya MPx* derived classes
//
//=============================================================================

#ifndef VSMAYAMPXFACTORY_H
#define VSMAYAMPXFACTORY_H
#if defined( _WIN32 )
#pragma once
#endif

#include <maya/MPxCommand.h>
#include <maya/MPxDeformerNode.h>
#include <maya/MPxDragAndDropBehavior.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MPxLocatorNode.h>
#include <maya/MPxNode.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MPxSurfaceShapeUI.h>
#include <maya/MPxToolCommand.h>
#include <maya/MPxTransform.h>
#include <maya/MPxTransformationMatrix.h>
#include <maya/MSyntax.h>

#if MAYA_API_VERSION >= 200800
#include <maya/MPxImageFile.h>
#endif //MAYA_API_VERSION >= 200800

//-----------------------------------------------------------------------------
//
// Forward declarations
//
//-----------------------------------------------------------------------------
class MFnPlugin;
namespace ValveMaya
{
	class CMSyntaxHelp;
}


//=============================================================================
//
// Base class for Maya MPx factories
//
//=============================================================================
class CVsMayaMPxFactoryBase
{
public:
	// Registers all MPx derived things that have been allocated
	static MStatus RegisterEverything( MFnPlugin &pluginFn );

	// Deregisters all MPx derived things that have been allocated
	static MStatus DeregisterEverything( MFnPlugin &pluginFn );

	// Displays a list of stuff in the plugin
	static void DisplaySummary( const MString &pluginName );

	// Types of things the MPxFactory can create
	enum Type
	{
		// NOTE: Ensure this list of enums stays in sync with GetTypeName() array
		kCommand,
		kFileTranslator,
		kDependencyNode,
		kShaderNode,
		kTransform,
		kLocatorNode,
		kImageFile,
		// Insert new ones above here
		kUnknown
	};

	void Enable( bool bEnabled ) { m_bEnabled = bEnabled; }

	bool IsEnabled() const { return m_bEnabled; }

protected:
	// Constructor
	CVsMayaMPxFactoryBase();

private:
	// The next factory
	CVsMayaMPxFactoryBase* m_pNextFactory;

	// The starting factory
	static CVsMayaMPxFactoryBase *s_pFirstFactory;

	// Register the thing associated with this factory
	virtual MStatus Register( MFnPlugin &pluginFn ) const = 0;

	// Deregister the thing associated with this factory
	virtual MStatus Deregister( MFnPlugin &pluginFn ) const = 0;

	// Everything has a name
	virtual const MString &GetName() const = 0;

	// Everything has a description
	virtual const MString &GetDesc() const = 0;

	// Everything has a type
	virtual Type GetType() const = 0;

	// Everything has a type (map types to names)
	MString GetTypeName() const;

	// Whether this factory is enabled or not
	bool m_bEnabled;
};


//-----------------------------------------------------------------------------
//
// Templatized helpers for creating MPx derived classes
//
//-----------------------------------------------------------------------------
template< class T >
class CVsMayaMPxFactory : public CVsMayaMPxFactoryBase
{
private:
	// Register the thing associated with this factory
	virtual MStatus Register( MFnPlugin &pluginFn ) const
	{
		return T::Register( pluginFn );
	}

	// Deregister the thing associated with this factory
	virtual MStatus Deregister( MFnPlugin &pluginFn ) const
	{
		return T::Deregister( pluginFn );
	}

	virtual const MString &GetName() const
	{
		return T::s_name;
	}

	virtual const MString &GetDesc() const
	{
		return T::s_desc;
	}

	virtual Type GetType() const
	{
		return T::GetType();
	}
};


//============================================================================
//
// Base class for Valve Maya commands ( CVsMayaMPxCommand )
//
//============================================================================
class CVsMayaMPxCommand : public MPxCommand
{
public:
	virtual const MString &GetName() const { return m_nullStr; }
	virtual const MString &GetDesc() const { return m_nullStr; }

protected:
	// Derived classes must specify this to override syntax
	virtual void SpecifySyntax( MSyntax &mSyntax, ValveMaya::CMSyntaxHelp &help );
	ValveMaya::CMSyntaxHelp *GetSyntaxHelp() { return m_pSyntaxHelp; }

private:
	ValveMaya::CMSyntaxHelp *m_pSyntaxHelp;

	static MStatus Register(
		MFnPlugin &pluginFn,
		const MString &name,
		MCreatorFunction creatorFunction,
		MCreateSyntaxFunction createSyntaxFunction = NULL );

	static MStatus Deregister( MFnPlugin &pluginFn, const MString &name );

	template < class T > friend class CVsMayaMPxCommandDecorator;

	MString m_nullStr;
};


//-----------------------------------------------------------------------------
//
// Decorator class for Valve Maya commands ( CVsMayaMPxCommandDecorator )
//
//-----------------------------------------------------------------------------
template < class T >
class CVsMayaMPxCommandDecorator : public T
{
public:
	static const MString &Name() { return s_name; };
	static const MString &Desc() { return s_desc; };

	virtual const MString &GetName() const { return Name(); };
	virtual const MString &GetDesc() const { return Desc(); };

	static CVsMayaMPxFactoryBase::Type GetType() { return CVsMayaMPxFactoryBase::kCommand; }

private:
	friend class CVsMayaMPxFactoryBase;
	template < class U > friend class CVsMayaMPxFactory;

	// These should be const but it's not because the CVsMayaMPxFactoryCommand class
	// only knows its name and therefore it's description at runtime

	static MString s_name;
	static MString s_desc;
	static ValveMaya::CMSyntaxHelp s_mSyntaxHelp;	// Keeps track of command line flags

	static void *Create()
	{
		CVsMayaMPxCommandDecorator *pDecorator = new CVsMayaMPxCommandDecorator< T >;
		pDecorator->m_pSyntaxHelp = &s_mSyntaxHelp;
		return pDecorator;
	}

	static MSyntax CreateSyntax()
	{
		// Maya will simply never call this unless the 'hasSyntax()' virtual returns true
		// doesn't matter if a syntaxCreator is registered or not, and an empty
		// MSyntax is fine too.  Also note the return is by value and not reference.
		// Also... even when Maya does call this, it is only ever called once, the
		// first time Maya needs to know what the syntax is (when the command is
		// invoked or when help cmd is done

		MSyntax mSyntax;
		T().SpecifySyntax( mSyntax, s_mSyntaxHelp );
		return mSyntax;
	}

	static MStatus Register( MFnPlugin &pluginFn )
	{
		return T::Register( pluginFn, s_name, Create, T().hasSyntax() ? CreateSyntax : NULL );
	}

	static MStatus Deregister( MFnPlugin &pluginFn )
	{
		return T::Deregister( pluginFn, s_name );
	}
};


//============================================================================
//
// Base class for Valve Maya commands ( CVsMayaMPxToolCommand )
//
//============================================================================
class CVsMayaMPxToolCommand : public MPxToolCommand
{
public:
	virtual const MString &GetName() const { return m_nullStr; }
	virtual const MString &GetDesc() const { return m_nullStr; }

protected:
	// Derived classes must specify this to override syntax
	virtual void SpecifySyntax( MSyntax &mSyntax, ValveMaya::CMSyntaxHelp &help );
	ValveMaya::CMSyntaxHelp *GetSyntaxHelp() { return m_pSyntaxHelp; }

private:
	ValveMaya::CMSyntaxHelp *m_pSyntaxHelp;

	static MStatus Register(
		MFnPlugin &pluginFn,
		const MString &name,
		MCreatorFunction creatorFunction,
		MCreateSyntaxFunction createSyntaxFunction = NULL );

	static MStatus Deregister( MFnPlugin &pluginFn, const MString &name );

	template < class T > friend class CVsMayaMPxToolCommandDecorator;

	MString m_nullStr;
};


//-----------------------------------------------------------------------------
//
// Decorator class for Valve Maya commands ( CVsMayaMPxToolCommandDecorator )
//
//-----------------------------------------------------------------------------
template < class T >
class CVsMayaMPxToolCommandDecorator : public T
{
public:
	static const MString &Name() { return s_name; };
	static const MString &Desc() { return s_desc; };

	virtual const MString &GetName() const { return Name(); };
	virtual const MString &GetDesc() const { return Desc(); };

	static CVsMayaMPxFactoryBase::Type GetType() { return CVsMayaMPxFactoryBase::kCommand; }

private:
	friend class CVsMayaMPxFactoryBase;
	template < class U > friend class CVsMayaMPxFactory;

	// These should be const but it's not because the CVsMayaMPxFactoryCommand class
	// only knows its name and therefore it's description at runtime

	static MString s_name;
	static MString s_desc;
	static ValveMaya::CMSyntaxHelp s_mSyntaxHelp;	// Keeps track of command line flags

	static void *Create()
	{
		CVsMayaMPxToolCommandDecorator *pDecorator = new CVsMayaMPxToolCommandDecorator< T >;
		pDecorator->m_pSyntaxHelp = &s_mSyntaxHelp;
		return pDecorator;
	}

	static MSyntax CreateSyntax()
	{
		// Maya will simply never call this unless the 'hasSyntax()' virtual returns true
		// doesn't matter if a syntaxCreator is registered or not, and an empty
		// MSyntax is fine too.  Also note the return is by value and not reference.
		// Also... even when Maya does call this, it is only ever called once, the
		// first time Maya needs to know what the syntax is (when the command is
		// invoked or when help cmd is done

		MSyntax mSyntax;
		T().SpecifySyntax( mSyntax, s_mSyntaxHelp );
		return mSyntax;
	}

	static MStatus Register( MFnPlugin &pluginFn )
	{
		return T::Register( pluginFn, s_name, Create, T().hasSyntax() ? CreateSyntax : NULL );
	}

	static MStatus Deregister( MFnPlugin &pluginFn )
	{
		return T::Deregister( pluginFn, s_name );
	}
};


//=============================================================================
//
// Base class for Valve Maya file translators ( CVsMayaMPxFileTranslator )
//
//=============================================================================
class CVsMayaMPxFileTranslator : public MPxFileTranslator
{
public:
	virtual const MString &GetName() const = 0;
	virtual const MString &GetGUIName() const = 0;

protected:
	static MStatus Register(
		MFnPlugin &pluginFn,
		const MString &name,
		const MString &guiName,
		MCreatorFunction creatorFunction );

	static MStatus Deregister(
		MFnPlugin &pluginFn,
		const MString &name );
};


//-----------------------------------------------------------------------------
//
// Decorator class for Valve Maya file translators ( CVsMayaMPxFileTranslatorDecorator )
//
//-----------------------------------------------------------------------------
template < class T >
class CVsMayaMPxFileTranslatorDecorator : public T
{
public:
	virtual const MString &GetName() const { return s_name; };

	virtual const MString &GetGUIName() const { return s_guiName; };

	virtual const MString &GetDesc() const { return s_desc; };

	static CVsMayaMPxFactoryBase::Type GetType() { return CVsMayaMPxFactoryBase::kFileTranslator; }

private:
	template < class U > friend class CVsMayaMPxFactory;

	static const MString s_name;

	static const MString s_desc;

	static const MString s_guiName;

	static void *Create()
	{
		return new CVsMayaMPxFileTranslatorDecorator< T >;
	}

	static MStatus Register( MFnPlugin &pluginFn )
	{
		return T::Register( pluginFn, s_name, s_guiName, Create );
	}

	static MStatus Deregister( MFnPlugin &pluginFn )
	{
		return T::Deregister( pluginFn, s_guiName );
	}
};


//=============================================================================
//
// Base class for Valve Maya Dependency Nodes ( CVsMayaMPxNode )
//
//============================================================================
class CVsMayaMPxNode : public MPxNode
{
public:
	virtual const MString &GetName() const = 0;

protected:
	static MStatus Register(
		MFnPlugin &pluginFn,
		const MString &name,
		const MTypeId &mTypeId,
		MCreatorFunction creatorFunction,
		MInitializeFunction initFunction,
		const MString &classification );

	static MStatus Deregister(
		MFnPlugin &pluginFn,
		const MTypeId &mTypeId );
};


//-----------------------------------------------------------------------------
//
// Decorator class for Valve Maya nodes ( CVsMayaMPxNodeDecorator )
//
//-----------------------------------------------------------------------------
template < class T >
class CVsMayaMPxNodeDecorator : public T
{
public:
	static const MString &Name() { return s_name; };

	virtual const MString &GetName() const { return Name(); };

	virtual const MString &GetDesc() const { return s_desc; };

	static CVsMayaMPxFactoryBase::Type GetType()
	{
		return s_classification.length() ? CVsMayaMPxFactoryBase::kShaderNode : CVsMayaMPxFactoryBase::kDependencyNode;
	}

private:
	template < class U > friend class CVsMayaMPxFactory;

	static const MString s_name;

	static const MString s_desc;

	static const MTypeId s_mTypeId;

	static const MInitializeFunction s_mInitializeFunction;

	static const MString s_classification;

	static void *Create()
	{
		return new CVsMayaMPxNodeDecorator< T >;
	}

	static MStatus Register( MFnPlugin &pluginFn )
	{
		return T::Register( pluginFn, s_name, s_mTypeId, Create, s_mInitializeFunction, s_classification );
	}

	static MStatus Deregister( MFnPlugin &pluginFn )
	{
		return T::Deregister( pluginFn, s_mTypeId );
	}
};


//=============================================================================
//
// Base class for Valve Maya Transform Nodes ( CVsMayaMPxTransform )
//
//============================================================================
class CVsMayaMPxTransform : public MPxTransform
{
public:
	virtual const MString &GetName() const = 0;

protected:

#if MAYA_API_VERSION >= 200900

	static MStatus Register(
		MFnPlugin &pluginFn,
		const MString &name,
		const MTypeId &mTypeId,
		MCreatorFunction creatorFunction,
		MInitializeFunction initFunction,
		MCreateXformMatrixFunction xformCreatorFunction = MPxTransformationMatrix::creator,
		const MTypeId &xformMTypeId = MPxTransformationMatrix::baseTransformationMatrixId,
		const MString *classification = NULL );

#else // #if MAYA_API_VERSION >= 200900

	static MStatus Register(
		MFnPlugin &pluginFn,
		const MString &name,
		const MTypeId &mTypeId,
		MCreatorFunction creatorFunction,
		MInitializeFunction initFunction,
		MCreatorFunction xformCreatorFunction = MPxTransformationMatrix::creator,
		const MTypeId &xformMTypeId = MPxTransformationMatrix::baseTransformationMatrixId,
		const MString *classification = NULL );

#endif // #if MAYA_API_VERSION >= 200900

	static MStatus Deregister(
		MFnPlugin &pluginFn,
		const MTypeId &mTypeId );
};


//-----------------------------------------------------------------------------
//
// Decorator class for Valve Maya commands ( CVsMayaMPxCommandDecorator )
//
//-----------------------------------------------------------------------------
template < class T >
class CVsMayaMPxTransformDecorator : public T
{
public:
	static const MString &Name() { return s_name; };

	virtual const MString &GetName() const { return Name(); };

	virtual const MString &GetDesc() const { return s_desc; };

	static CVsMayaMPxFactoryBase::Type GetType() { return CVsMayaMPxFactoryBase::kTransform; }

private:
	template < class U > friend class CVsMayaMPxFactory;

	static const MString s_name;

	static const MString s_desc;

	static const MTypeId s_mTypeId;

	static const MInitializeFunction s_mInitializeFunction;

#if MAYA_API_VERSION >= 200900

	static const MCreateXformMatrixFunction s_xformMCreatorFunction;

#else // #if MAYA_API_VERSION >= 200900

	static const MCreatorFunction s_xformMCreatorFunction;

#endif // #if MAYA_API_VERSION >= 200900

	static const MTypeId s_xformMTypeId;

	static void *Create()
	{
		return new CVsMayaMPxTransformDecorator< T >;
	}

	static MStatus Register( MFnPlugin &pluginFn )
	{
		return T::Register( pluginFn, s_name, s_mTypeId, Create, s_mInitializeFunction, s_xformMCreatorFunction, s_xformMTypeId );
	}

	static MStatus Deregister( MFnPlugin &pluginFn )
	{
		return T::Deregister( pluginFn, s_mTypeId );
	}
};


//=============================================================================
//
// Base class for Valve Maya Locator Nodes ( CVsMayaMPxLocatorNode )
//
//============================================================================
class CVsMayaMPxLocatorNode : public MPxLocatorNode
{
public:
	virtual const MString &GetName() const = 0;

protected:
	static MStatus Register(
		MFnPlugin &pluginFn,
		const MString &name,
		const MTypeId &mTypeId,
		MCreatorFunction creatorFunction,
		MInitializeFunction initFunction );

	static MStatus Deregister(
		MFnPlugin &pluginFn,
		const MTypeId &mTypeId );
};


//-----------------------------------------------------------------------------
//
// Decorator class for Valve Maya nodes ( CVsMayaMPxLocatorNodeDecorator )
//
//-----------------------------------------------------------------------------
template < class T >
class CVsMayaMPxLocatorNodeDecorator : public T
{
public:
	static const MString &Name() { return s_name; };

	virtual const MString &GetName() const { return Name(); };

	virtual const MString &GetDesc() const { return s_desc; };

	static CVsMayaMPxFactoryBase::Type GetType()
	{
		return CVsMayaMPxFactoryBase::kLocatorNode;
	}

private:
	template < class U > friend class CVsMayaMPxFactory;

	static const MString s_name;

	static const MString s_desc;

	static const MTypeId s_mTypeId;

	static const MInitializeFunction s_mInitializeFunction;

	static void *Create()
	{
		return new CVsMayaMPxLocatorNodeDecorator< T >;
	}

	static MStatus Register( MFnPlugin &pluginFn )
	{
		return T::Register( pluginFn, s_name, s_mTypeId, Create, s_mInitializeFunction );
	}

	static MStatus Deregister( MFnPlugin &pluginFn )
	{
		return T::Deregister( pluginFn, s_mTypeId );
	}
};


//=============================================================================
//
// Base class for Valve Maya Drag And Drop Behaviors ( CVsMayaMPxDragAndDropBehavior )
//
//============================================================================
class CVsMayaMPxDragAndDropBehavior : public MPxDragAndDropBehavior
{
public:
	virtual const MString &GetName() const = 0;

protected:
	static MStatus Register(
		MFnPlugin &pluginFn,
		const MString &name,
		MCreatorFunction creatorFunction );

	static MStatus Deregister(
		MFnPlugin &pluginFn,
		const MString &name );
};


//-----------------------------------------------------------------------------
//
// Decorator class for Valve Maya drag and drop behaviors ( CVsMayaMPxDragAndDropBehavior )
//
//-----------------------------------------------------------------------------
template < class T >
class CVsMayaMPxDragAndDropBehaviorDecorator : public T
{
public:
	static const MString &Name() { return s_name; };

	virtual const MString &GetName() const { return Name(); };

	virtual const MString &GetDesc() const { return s_desc; };

	static CVsMayaMPxFactoryBase::Type GetType()
	{
		return CVsMayaMPxFactoryBase::kLocatorNode;
	}

private:
	template < class U > friend class CVsMayaMPxFactory;

	static const MString s_name;

	static const MString s_desc;

	static const MInitializeFunction s_mInitializeFunction;

	static void *Create()
	{
		return new CVsMayaMPxDragAndDropBehaviorDecorator< T >;
	}

	static MStatus Register( MFnPlugin &pluginFn )
	{
		return T::Register( pluginFn, s_name, Create );
	}

	static MStatus Deregister( MFnPlugin &pluginFn )
	{
		return T::Deregister( pluginFn, s_name );
	}
};


//=============================================================================
//
// Base class for Valve Maya Shape Nodes ( CVsMayaMPxShapeNode )
//
//============================================================================
class CVsMayaMPxShapeNode : public MPxSurfaceShape
{
public:
	virtual const MString &GetName() const = 0;

protected:
	static MStatus Register(
		MFnPlugin &pluginFn,
		const MString &name,
		const MTypeId &mTypeId,
		MCreatorFunction creatorFunction,
		MInitializeFunction initFunction,
		MCreatorFunction uiCreatorFunction );

	static MStatus Deregister(
		MFnPlugin &pluginFn,
		const MTypeId &mTypeId );
};


//-----------------------------------------------------------------------------
//
// Decorator class for Valve Maya shape nodes ( CVsMayaMPxShapeNodeDecorator )
//
//-----------------------------------------------------------------------------
template < class T, class U >
class CVsMayaMPxShapeNodeDecorator : public T
{
public:
	static const MString &Name() { return s_name; };

	virtual const MString &GetName() const { return Name(); };

	virtual const MString &GetDesc() const { return s_desc; };

	static CVsMayaMPxFactoryBase::Type GetType()
	{
		return CVsMayaMPxFactoryBase::kLocatorNode;
	}

private:
	template < class U > friend class CVsMayaMPxFactory;

	static const MString s_name;

	static const MString s_desc;

	static const MTypeId s_mTypeId;

	static const MInitializeFunction s_mInitializeFunction;

	static const MCreatorFunction s_uiCreatorFunction;

	static void *Create()
	{
		return new CVsMayaMPxShapeNodeDecorator< T, U >;
	}

	static void *CreateUI()
	{
		return new U;
	}

	static MStatus Register( MFnPlugin &pluginFn )
	{
		return T::Register( pluginFn, s_name, s_mTypeId, Create, s_mInitializeFunction, CreateUI );
	}

	static MStatus Deregister( MFnPlugin &pluginFn )
	{
		return T::Deregister( pluginFn, s_mTypeId );
	}
};

#if MAYA_API_VERSION >= 200800
//=============================================================================
//
// Base class for Valve Maya Image File Types ( CVsMayaMPxImageFile )
//
//============================================================================
class CVsMayaMPxImageFile : public MPxImageFile
{
public:
	virtual const MString &GetName() const = 0;

protected:
	static MStatus Register(
		MFnPlugin &pluginFn,
		const MString &name,
		MCreatorFunction creatorFunction,
		const MStringArray &extensions );

	static MStatus Deregister(
		MFnPlugin &pluginFn,
		const MString &name );
};


//-----------------------------------------------------------------------------
//
// Decorator class for Valve Maya Image Files ( CVsMayaMPxImageFileDecorator )
//
//-----------------------------------------------------------------------------
template < class T >
class CVsMayaMPxImageFileDecorator : public T
{
public:
	static const MString &Name() { return s_name; };

	virtual const MString &GetName() const { return Name(); };

	virtual const MString &GetDesc() const { return s_desc; };

	static CVsMayaMPxFactoryBase::Type GetType()
	{
		return CVsMayaMPxFactoryBase::kImageFile;
	}

private:
	template < class T > friend class CVsMayaMPxFactory;

	static const MString s_name;

	static const MString s_desc;

	static const MStringArray s_extensions;

	static const MCreatorFunction s_creatorFunction;

	static void *Create()
	{
		return new CVsMayaMPxImageFileDecorator< T >;
	}

	static MStatus Register( MFnPlugin &pluginFn )
	{
		return T::Register( pluginFn, s_name, Create, s_extensions );
	}

	static MStatus Deregister( MFnPlugin &pluginFn )
	{
		return T::Deregister( pluginFn, s_name );
	}
};


//-----------------------------------------------------------------------------
// Helper macro to instantiate an image file
//-----------------------------------------------------------------------------
#define INSTALL_MAYA_MPXIMAGEFILE( _class, _name, _extensions, _desc )									\
	const MString CVsMayaMPxImageFileDecorator< _class >::s_name( #_name );											\
	const MString CVsMayaMPxImageFileDecorator< _class >::s_desc( _desc );											\
	const MStringArray CVsMayaMPxImageFileDecorator< _class >::s_extensions( _extensions );										\
	static CVsMayaMPxFactory< CVsMayaMPxImageFileDecorator< _class > > s_##_name##_Factory


#endif // MAYA_API_VERSION >= 200800


//=============================================================================
//
// Base class for Valve Maya Dependency Nodes ( CVsMayaMPxNode )
//
//============================================================================
class CVsMayaMPxDeformerNode : public MPxDeformerNode
{
public:
	virtual const MString &GetName() const = 0;

protected:
	static MStatus Register(
		MFnPlugin &pluginFn,
		const MString &name,
		const MTypeId &mTypeId,
		MCreatorFunction creatorFunction,
		MInitializeFunction initFunction,
		const MString &classification );

	static MStatus Deregister(
		MFnPlugin &pluginFn,
		const MTypeId &mTypeId );
};


//-----------------------------------------------------------------------------
//
// Decorator class for Valve Maya nodes ( CVsMayaMPxDeformerNodeDecorator )
//
//-----------------------------------------------------------------------------
template < class T >
class CVsMayaMPxDeformerNodeDecorator : public T
{
public:
	static const MString &Name() { return s_name; };

	virtual const MString &GetName() const { return Name(); };

	virtual const MString &GetDesc() const { return s_desc; };

	static CVsMayaMPxFactoryBase::Type GetType()
	{
		return s_classification.length() ? CVsMayaMPxFactoryBase::kShaderNode : CVsMayaMPxFactoryBase::kDependencyNode;
	}

private:
	template < class U > friend class CVsMayaMPxFactory;

	static const MString s_name;

	static const MString s_desc;

	static const MTypeId s_mTypeId;

	static const MInitializeFunction s_mInitializeFunction;

	static const MString s_classification;

	static void *Create()
	{
		return new CVsMayaMPxDeformerNodeDecorator< T >;
	}

	static MStatus Register( MFnPlugin &pluginFn )
	{
		return T::Register( pluginFn, s_name, s_mTypeId, Create, s_mInitializeFunction, s_classification );
	}

	static MStatus Deregister( MFnPlugin &pluginFn )
	{
		return T::Deregister( pluginFn, s_mTypeId );
	}
};


//=============================================================================
//
// Helper Macros
//
//=============================================================================

//-----------------------------------------------------------------------------
// Helper macro to instantiate a command 
//-----------------------------------------------------------------------------
#define INSTALL_MAYA_MPXCOMMAND( _class, _name, _desc )										\
	MString CVsMayaMPxCommandDecorator< _class >::s_name( #_name );							\
	MString CVsMayaMPxCommandDecorator< _class >::s_desc( _desc );							\
	ValveMaya::CMSyntaxHelp CVsMayaMPxCommandDecorator< _class >::s_mSyntaxHelp;			\
	static CVsMayaMPxFactory< CVsMayaMPxCommandDecorator< _class > > s_##_name##_Factory

//-----------------------------------------------------------------------------
// Helper macro to instantiate a command 
//-----------------------------------------------------------------------------
#define INSTALL_MAYA_MPXTOOLCOMMAND( _class, _name, _desc )										\
	MString CVsMayaMPxToolCommandDecorator< _class >::s_name( #_name );							\
	MString CVsMayaMPxToolCommandDecorator< _class >::s_desc( _desc );							\
	ValveMaya::CMSyntaxHelp CVsMayaMPxToolCommandDecorator< _class >::s_mSyntaxHelp;			\
	static CVsMayaMPxFactory< CVsMayaMPxToolCommandDecorator< _class > > s_##_name##_Factory


//-----------------------------------------------------------------------------
// Helper macro to instantiate a translator 
//-----------------------------------------------------------------------------
#define INSTALL_MAYA_MPXFILETRANSLATOR( _class, _name, _guiName, _desc )				\
	const MString CVsMayaMPxFileTranslatorDecorator< _class >::s_name( #_name );		\
	const MString CVsMayaMPxFileTranslatorDecorator< _class >::s_desc( _desc );			\
	const MString CVsMayaMPxFileTranslatorDecorator< _class >::s_guiName( _guiName );	\
	static CVsMayaMPxFactory< CVsMayaMPxFileTranslatorDecorator< _class > > s_##_name##_Factory


//-----------------------------------------------------------------------------
// Helper macro to instantiate a regular dependency node 
//-----------------------------------------------------------------------------
#define INSTALL_MAYA_MPXNODE( _class, _name, _typeId, _initializeFunction, _desc )								\
	const MString CVsMayaMPxNodeDecorator< _class >::s_name( #_name );											\
	const MString CVsMayaMPxNodeDecorator< _class >::s_desc( _desc );											\
	const MTypeId CVsMayaMPxNodeDecorator< _class >::s_mTypeId( _typeId );										\
	const MInitializeFunction CVsMayaMPxNodeDecorator< _class >::s_mInitializeFunction( _initializeFunction );	\
	const MString CVsMayaMPxNodeDecorator< _class >::s_classification( "" );									\
	static CVsMayaMPxFactory< CVsMayaMPxNodeDecorator< _class > > s_##_name##_Factory


//-----------------------------------------------------------------------------
// Helper macro to instantiate a shader dependency node 
//-----------------------------------------------------------------------------
#define INSTALL_MAYA_MPXSHADERNODE( _class, _name, _typeId, _initializeFunction, _classification, _desc )		\
	const MString CVsMayaMPxNodeDecorator< _class >::s_name( #_name );											\
	const MString CVsMayaMPxNodeDecorator< _class >::s_desc( _desc );											\
	const MTypeId CVsMayaMPxNodeDecorator< _class >::s_mTypeId( _typeId );										\
	const MInitializeFunction CVsMayaMPxNodeDecorator< _class >::s_mInitializeFunction( _initializeFunction );	\
	const MString CVsMayaMPxNodeDecorator< _class >::s_classification( _classification );						\
	static CVsMayaMPxFactory< CVsMayaMPxNodeDecorator< _class > > s_##_name##_Factory


//-----------------------------------------------------------------------------
// Helper macro to instantiate a transform node 
//-----------------------------------------------------------------------------
#if MAYA_API_VERSION >= 200900

#define INSTALL_MAYA_MPXTRANSFORM( _class, _name, _typeId, _initializeFunction, _desc )													\
	const MString CVsMayaMPxTransformDecorator< _class >::s_name( #_name );																\
	const MString CVsMayaMPxTransformDecorator< _class >::s_desc( _desc );																\
	const MTypeId CVsMayaMPxTransformDecorator< _class >::s_mTypeId( _typeId );															\
	const MInitializeFunction CVsMayaMPxTransformDecorator< _class >::s_mInitializeFunction( _initializeFunction );						\
	const MCreateXformMatrixFunction CVsMayaMPxTransformDecorator< _class >::s_xformMCreatorFunction( MPxTransformationMatrix::creator );			\
	const MTypeId CVsMayaMPxTransformDecorator< _class >::s_xformMTypeId( MPxTransformationMatrix::baseTransformationMatrixId );		\
	static CVsMayaMPxFactory< CVsMayaMPxTransformDecorator< _class > > s_##_name##_Factory

#else // #if MAYA_API_VERSION >= 200900

#define INSTALL_MAYA_MPXTRANSFORM( _class, _name, _typeId, _initializeFunction, _desc )													\
	const MString CVsMayaMPxTransformDecorator< _class >::s_name( #_name );																\
	const MString CVsMayaMPxTransformDecorator< _class >::s_desc( _desc );																\
	const MTypeId CVsMayaMPxTransformDecorator< _class >::s_mTypeId( _typeId );															\
	const MInitializeFunction CVsMayaMPxTransformDecorator< _class >::s_mInitializeFunction( _initializeFunction );						\
	const MCreatorFunction CVsMayaMPxTransformDecorator< _class >::s_xformMCreatorFunction( MPxTransformationMatrix::creator );			\
	const MTypeId CVsMayaMPxTransformDecorator< _class >::s_xformMTypeId( MPxTransformationMatrix::baseTransformationMatrixId );		\
	static CVsMayaMPxFactory< CVsMayaMPxTransformDecorator< _class > > s_##_name##_Factory

#endif // #if MAYA_API_VERSION >= 200900


//-----------------------------------------------------------------------------
// Helper macro to instantiate a transform node with a custom transformation matrix
// TODO: Make CVsMayaMPxTransformationMatrix and create the MCreatorFunction for the user
//-----------------------------------------------------------------------------
#if MAYA_API_VERSION >= 200900

#define INSTALL_MAYA_MPXTRANSFORM_WITHMATRIX( _class, _name, _typeId, _initializeFunction, _xformCreatorFunction, _xformTypeId, _desc )	\
	const MString CVsMayaMPxTransformDecorator< _class >::s_name( #_name );																\
	const MString CVsMayaMPxTransformDecorator< _class >::s_desc( _desc );																\
	const MTypeId CVsMayaMPxTransformDecorator< _class >::s_mTypeId( _typeId );															\
	const MInitializeFunction CVsMayaMPxTransformDecorator< _class >::s_mInitializeFunction( _initializeFunction );						\
	const MCreateXformMatrixFunction CVsMayaMPxTransformDecorator< _class >::s_xformMCreatorFunction( _xformCreatorFunction );					\
	const MTypeId CVsMayaMPxTransformDecorator< _class >::s_xformMTypeId( _xformTypeId );												\
	static CVsMayaMPxFactory< CVsMayaMPxTransformDecorator< _class > > s_##_name##_Factory

#else // #if MAYA_API_VERSION >= 200900

#define INSTALL_MAYA_MPXTRANSFORM_WITHMATRIX( _class, _name, _typeId, _initializeFunction, _xformCreatorFunction, _xformTypeId, _desc )	\
	const MString CVsMayaMPxTransformDecorator< _class >::s_name( #_name );																\
	const MString CVsMayaMPxTransformDecorator< _class >::s_desc( _desc );																\
	const MTypeId CVsMayaMPxTransformDecorator< _class >::s_mTypeId( _typeId );															\
	const MInitializeFunction CVsMayaMPxTransformDecorator< _class >::s_mInitializeFunction( _initializeFunction );						\
	const MCreatorFunction CVsMayaMPxTransformDecorator< _class >::s_xformMCreatorFunction( _xformCreatorFunction );					\
	const MTypeId CVsMayaMPxTransformDecorator< _class >::s_xformMTypeId( _xformTypeId );												\
	static CVsMayaMPxFactory< CVsMayaMPxTransformDecorator< _class > > s_##_name##_Factory

#endif // #if MAYA_API_VERSION >= 200900


//-----------------------------------------------------------------------------
// Helper macro to instantiate a locator node 
//-----------------------------------------------------------------------------
#define INSTALL_MAYA_MPXLOCATORNODE( _class, _name, _typeId, _initializeFunction, _desc )								\
	const MString CVsMayaMPxLocatorNodeDecorator< _class >::s_name( #_name );											\
	const MString CVsMayaMPxLocatorNodeDecorator< _class >::s_desc( _desc );											\
	const MTypeId CVsMayaMPxLocatorNodeDecorator< _class >::s_mTypeId( _typeId );										\
	const MInitializeFunction CVsMayaMPxLocatorNodeDecorator< _class >::s_mInitializeFunction( _initializeFunction );	\
	static CVsMayaMPxFactory< CVsMayaMPxLocatorNodeDecorator< _class > > s_##_name##_Factory


//-----------------------------------------------------------------------------
// Helper macro to instantiate a drag and drop behavior
//-----------------------------------------------------------------------------
#define INSTALL_MAYA_MPXDRAGANDDROPBEHAVIOR( _class, _name, _desc )										\
	const MString CVsMayaMPxDragAndDropBehaviorDecorator< _class >::s_name( #_name );					\
	const MString CVsMayaMPxDragAndDropBehaviorDecorator< _class >::s_desc( _desc );					\
	static CVsMayaMPxFactory< CVsMayaMPxDragAndDropBehaviorDecorator< _class > > s_##_name##_Factory


//-----------------------------------------------------------------------------
// Helper macro to instantiate a shape node 
//-----------------------------------------------------------------------------
#define INSTALL_MAYA_MPXSHAPENODE( _class, _name, _typeId, _initializeFunction, _uiClass, _desc )									\
	const MString CVsMayaMPxShapeNodeDecorator< _class, _uiClass >::s_name( #_name );											\
	const MString CVsMayaMPxShapeNodeDecorator< _class, _uiClass >::s_desc( _desc );											\
	const MTypeId CVsMayaMPxShapeNodeDecorator< _class, _uiClass >::s_mTypeId( _typeId );										\
	const MInitializeFunction CVsMayaMPxShapeNodeDecorator< _class, _uiClass >::s_mInitializeFunction( _initializeFunction );	\
	static CVsMayaMPxFactory< CVsMayaMPxShapeNodeDecorator< _class, _uiClass > > s_##_name##_Factory


//-----------------------------------------------------------------------------
// Helper macro to instantiate a deformer dependency node 
//-----------------------------------------------------------------------------
#define INSTALL_MAYA_MPXDEFORMERNODE( _class, _name, _typeId, _initializeFunction, _desc )								\
	const MString CVsMayaMPxDeformerNodeDecorator< _class >::s_name( #_name );											\
	const MString CVsMayaMPxDeformerNodeDecorator< _class >::s_desc( _desc );											\
	const MTypeId CVsMayaMPxDeformerNodeDecorator< _class >::s_mTypeId( _typeId );										\
	const MInitializeFunction CVsMayaMPxDeformerNodeDecorator< _class >::s_mInitializeFunction( _initializeFunction );	\
	const MString CVsMayaMPxDeformerNodeDecorator< _class >::s_classification( "" );									\
	static CVsMayaMPxFactory< CVsMayaMPxDeformerNodeDecorator< _class > > s_##_name##_Factory


#endif // VSMAYAMPXFACTORY_H