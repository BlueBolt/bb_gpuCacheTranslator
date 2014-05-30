/* (c)2012 BlueBolt Ltd. All rights reserved.
 *
 * BB_gpuCacheTranslator.cpp
 *
 *  Created on: 20 Jul 2012
 *      Author: ashley-r
 */

#include <maya/MFnDagNode.h>
#include <maya/MBoundingBox.h>
#include <maya/MPlugArray.h>

#include <extension/Extension.h>
#include <translators/shape/ShapeTranslator.h>

// #include "bb_alembicArchiveShape.h"

/*
 * Return a new string with all occurrences of 'from' replaced with 'to'
 */
std::string replace_all(const MString &str, const char *from, const char *to)
{
    std::string result(str.asChar());
    std::string::size_type
        index = 0,
        from_len = strlen(from),
        to_len = strlen(to);
    while ((index = result.find(from, index)) != std::string::npos) {
        result.replace(index, from_len, to);
        index += to_len;
    }
    return result;
}

class BB_gpuCacheTranslator : public CShapeTranslator
{

        public :

                virtual AtNode *CreateArnoldNodes()
                {
                  m_isMasterDag =  IsMasterInstance();
                  m_masterDag = GetMasterInstance();
                  if (m_isMasterDag)
                  {
                      return AddArnoldNode( "procedural" );
                  }
                  else
                  {
                      return AddArnoldNode( "ginstance" );
                  }
                }



                virtual void Export( AtNode* instance )
                {
                  Update(instance);
                }

                virtual void Update(AtNode* node)
                {
                   const char* nodeType = AiNodeEntryGetName(AiNodeGetNodeEntry(node));
                   if (strcmp(nodeType, "ginstance") == 0)
                   {
                      ExportInstance(node, m_masterDag);
                   }
                   else
                   {
                      ExportProcedural(node);
                   }
                }

                virtual AtNode* ExportInstance(AtNode *instance, const MDagPath& masterInstance)
                {
                   AtNode* masterNode = AiNodeLookUpByName(masterInstance.partialPathName().asChar());


                   int instanceNum = m_dagPath.instanceNumber();

                   if ( instanceNum > 0 )
                     {
                       std::cout << "ExportInstance::instanceNum :: " << instanceNum << std::endl;

//                       char nodeName[65535];
                       AiNodeSetStr(instance, "name", m_dagPath.partialPathName().asChar());

                       ExportMatrix(instance, 0);

                       AiNodeSetPtr(instance, "node", masterNode);
                       AiNodeSetBool(instance, "inherit_xform", false);
                       int visibility = AiNodeGetInt(masterNode, "visibility");
                       AiNodeSetInt(instance, "visibility", visibility);

                       AiNodeSetPtr( instance, "shader", arnoldShader(instance) );

                       // Export light linking per instance
                       ExportLightLinking(instance);
                     }
                   return instance;
                }

                virtual void ExportProcedural( AtNode *node )
                {
                        // do basic node export
                        ExportMatrix( node, 0 );

                        // AiNodeSetPtr( node, "shader", arnoldShader(node) );


                        AiNodeSetInt( node, "visibility", ComputeVisibility() );

                        MPlug plug = FindMayaObjectPlug( "receiveShadows" );
                        if( !plug.isNull() )
                        {
                                AiNodeSetBool( node, "receive_shadows", plug.asBool() );
                        }

                        plug = FindMayaObjectPlug( "aiSelfShadows" );
                        if( !plug.isNull() )
                        {
                                AiNodeSetBool( node, "self_shadows", plug.asBool() );
                        }

                        plug = FindMayaObjectPlug( "aiOpaque" );
                        if( !plug.isNull() )
                        {
                                AiNodeSetBool( node, "opaque", plug.asBool() );
                        }

                        // now set the procedural-specific parameters

                        AiNodeSetBool( node, "load_at_init", true ); // just for now so that it can load the shaders at the right time

                        MFnDagNode fnDagNode( m_dagPath );
                        MBoundingBox bound = fnDagNode.boundingBox();

                        AiNodeSetPnt( node, "min", bound.min().x-m_dispPadding, bound.min().y-m_dispPadding, bound.min().z-m_dispPadding );
                        AiNodeSetPnt( node, "max", bound.max().x+m_dispPadding, bound.max().y, bound.max().z+m_dispPadding );

                        const char *dsoPath = getenv( "ALEMBIC_ARNOLD_PROCEDURAL_PATH" );
                        AiNodeSetStr( node, "dso",  dsoPath ? dsoPath : "bb_AlembicArnoldProcedural.so" );

                        // Set the parameters for the procedural

                        //abcFile path
                        MString abcFile = fnDagNode.findPlug("cacheFileName").asString().expandEnvironmentVariablesAndTilde();

                        //object path
                        MString objectPath = fnDagNode.findPlug("cacheGeomPath").asString();

                        //object pattern
                        MString objectPattern = "*";

                        plug = FindMayaObjectPlug( "objectPattern" );
                        if (!plug.isNull() )
                        {
                              if (plug.asString() != "")
                              {
                                objectPattern = plug.asString();
                              }
                        }

                        float shutterOpen = 0.0;
                        plug = FindMayaObjectPlug( "shutterOpen" );
                        if (!plug.isNull() )
                        {
                                shutterOpen = plug.asFloat();
                        }

                        float shutterClose = 0.0;
                        plug = FindMayaObjectPlug( "shutterClose" );
                        if (!plug.isNull() )
                        {
                                shutterClose = plug.asFloat();
                        }

                        float timeOffset = 0.0;
                        plug = FindMayaObjectPlug( "timeOffset" );
                        if (!plug.isNull() )
                        {
                                timeOffset = plug.asFloat();
                        }

                        int subDIterations = 0;
                        plug = FindMayaObjectPlug( "ai_subDIterations" );
                        if (!plug.isNull() )
                        {
                                subDIterations = plug.asInt();
                        }

                        MString nameprefix = "";
                        plug = FindMayaObjectPlug( "namePrefix" );
                        if (!plug.isNull() )
                        {
                                nameprefix = plug.asString();
                        }

                        // bool exportFaceIds = fnDagNode.findPlug("exportFaceIds").asBool();

                        bool makeInstance = true; // always on for now
                        plug = FindMayaObjectPlug( "makeInstance" );
                        if (!plug.isNull() )
                        {
                                makeInstance = plug.asBool();
                        }
                        
                        bool flipv = false; 
                        plug = FindMayaObjectPlug( "flipv" );
                        if (!plug.isNull() )
                        {
                                flipv = plug.asBool();
                        }

                        bool invertNormals = false; 
                        plug = FindMayaObjectPlug( "invertNormals" );
                        if (!plug.isNull() )
                        {
                                invertNormals = plug.asBool();
                        }
                        
                        short i_subDUVSmoothing = 1;
                        plug = FindMayaObjectPlug( "ai_subDUVSmoothing" );
                        if (!plug.isNull() )
                        {
                                i_subDUVSmoothing = plug.asShort();
                        }

                        MString  subDUVSmoothing;

                        switch (i_subDUVSmoothing)
                        {
                          case 0:
                            subDUVSmoothing = "pin_corners";
                            break;
                          case 1:
                            subDUVSmoothing = "pin_borders";
                            break;
                          case 2:
                            subDUVSmoothing = "linear";
                            break;
                          case 3:
                            subDUVSmoothing = "smooth";
                            break;
                          default :
                            subDUVSmoothing = "pin_corners";
                            break;
                        }

                        MTime curTime = MAnimControl::currentTime();
                        // fnDagNode.findPlug("time").getValue( frame );

                        // MTime frameOffset;
                        // fnDagNode.findPlug("timeOffset").getValue( frameOffset );

                        float time = curTime.as(MTime::kFilm)+timeOffset;

                        MString argsString;
                        if (objectPath != "|"){
                                argsString += "-objectpath ";
                                // convert "|" to "/"

                                argsString += MString(replace_all(objectPath,"|","/").c_str());
                        }
                        if (objectPattern != "*"){
                                argsString += "-pattern ";
                                argsString += objectPattern;
                        }
                        if (shutterOpen != 0.0){
                                argsString += " -shutteropen ";
                                argsString += shutterOpen;
                        }
                        if (shutterClose != 0.0){
                                argsString += " -shutterclose ";
                                argsString += shutterClose;
                        }
                        if (subDIterations != 0){
                                argsString += " -subditerations ";
                                argsString += subDIterations;
                                argsString += " -subduvsmoothing ";
                                argsString += subDUVSmoothing;
                        }
                        if (makeInstance){
                                argsString += " -makeinstance ";
                        }
                        if (nameprefix != ""){
                                argsString += " -nameprefix ";
                                argsString += nameprefix;
                        }
                        if (flipv){
                                argsString += " -flipv ";
                        }
                        if (invertNormals){
                                argsString += " -invertNormals ";
                        }
                        argsString += " -filename ";
                        argsString += abcFile;
                        argsString += " -frame ";
                        argsString += time;

                        if (m_displaced){

                            argsString += " -disp_map ";
                            argsString += AiNodeGetName(m_dispNode);

                        }

                        AiNodeSetStr(node, "data", argsString.asChar());

                        ExportUserAttrs(node);

                        // Export light linking per instance
                        ExportLightLinking(node);

                }

                virtual void ExportUserAttrs( AtNode *node )
                {
                        // Get the optional attributes and export them as user vars

                        MPlug plug = FindMayaObjectPlug( "shaderAssignation" );
                        if( !plug.isNull() )
                        {
                                AiNodeDeclare( node, "shaderAssignation", "constant STRING" );
                                AiNodeSetStr( node, "shaderAssignation", plug.asString().asChar() );
                        }

                        plug = FindMayaObjectPlug( "displacementAssignation" );
                        if( !plug.isNull() )
                        {
                                AiNodeDeclare( node, "displacementAssignation", "constant STRING" );
                                AiNodeSetStr( node, "displacementAssignation", plug.asString().asChar() );
                        }

                        plug = FindMayaObjectPlug( "shaderAssignmentfile" );
                        if( !plug.isNull() )
                        {
                                AiNodeDeclare( node, "shaderAssignmentfile", "constant STRING" );
                                AiNodeSetStr( node, "shaderAssignmentfile", plug.asString().asChar() );
                        }

                        plug = FindMayaObjectPlug( "overrides" );
                        if( !plug.isNull() )
                        {
                                AiNodeDeclare( node, "overrides", "constant STRING" );
                                AiNodeSetStr( node, "overrides", plug.asString().asChar() );
                        }

                        plug = FindMayaObjectPlug( "overridefile" );
                        if( !plug.isNull() )
                        {
                                AiNodeDeclare( node, "overridefile", "constant STRING" );
                                AiNodeSetStr( node, "overridefile", plug.asString().asChar() );
                        }

                        plug = FindMayaObjectPlug( "userAttributes" );
                        if( !plug.isNull() )
                        {
                                AiNodeDeclare( node, "userAttributes", "constant STRING" );
                                AiNodeSetStr( node, "userAttributes", plug.asString().asChar() );
                        }

                        plug = FindMayaObjectPlug( "userAttributesfile" );
                        if( !plug.isNull() )
                        {
                                AiNodeDeclare( node, "userAttributesfile", "constant STRING" );
                                AiNodeSetStr( node, "userAttributesfile", plug.asString().asChar() );
                        }

                        plug = FindMayaObjectPlug( "skipJson" );
                        if( !plug.isNull() )
                        {
                                AiNodeDeclare( node, "skipJson", "constant BOOL" );   
                                AiNodeSetBool( node, "skipJson", plug.asBool() );
                        }

                        plug = FindMayaObjectPlug( "skipShaders" );
                        if( !plug.isNull() )
                        {
                                AiNodeDeclare( node, "skipShaders", "constant BOOL" );   
                                AiNodeSetBool( node, "skipShaders", plug.asBool() );
                        }

                        plug = FindMayaObjectPlug( "skipOverrides" );
                        if( !plug.isNull() )
                        {
                                AiNodeDeclare( node, "skipOverrides", "constant BOOL" );   
                                AiNodeSetBool( node, "skipOverrides", plug.asBool() );
                        }

                        plug = FindMayaObjectPlug( "skipUserAttributes" );
                        if( !plug.isNull() )
                        {
                                AiNodeDeclare( node, "skipUserAttributes", "constant BOOL" );   
                                AiNodeSetBool( node, "skipUserAttributes", plug.asBool() );
                        }

                        plug = FindMayaObjectPlug( "skipDisplacements" );
                        if( !plug.isNull() )
                        {
                                AiNodeDeclare( node, "skipDisplacements", "constant BOOL" );                          
                                AiNodeSetBool( node, "skipDisplacements", plug.asBool() );
                        }

                        plug = FindMayaObjectPlug( "objectPattern" );
                        if( !plug.isNull() )
                        {
                                AiNodeDeclare( node, "objectPattern", "constant STRING" );
                                AiNodeSetStr( node, "objectPattern", plug.asString().asChar() );
                        }
                       
                        
                        plug = FindMayaObjectPlug( "assShaders" );
                        if( !plug.isNull() )
                        {
                                AiNodeDeclare( node, "assShaders", "constant STRING" );
                                AiNodeSetStr( node, "assShaders", plug.asString().asChar() );
                        }

                }


                virtual bool RequiresMotionData()
                {
                        return IsMotionBlurEnabled( MTOA_MBLUR_OBJECT ) && IsLocalMotionBlurEnabled();
                }

                virtual void ExportMotion( AtNode *node, unsigned int step )
                {
                        if( !IsMotionBlurEnabled() )
                        {
                                return;
                        }

                        ExportMatrix( node, step );
                }

                static void nodeInitialiser( CAbTranslator context )
                {
                        CExtensionAttrHelper helper( context.maya, "procedural" );

                        CAttrData data;
                        data.stringDefault = "";
                        data.name = "aiTraceSets";
                        data.shortName = "trace_sets";
                        data.type = AI_TYPE_STRING;

                        helper.MakeInput(data);

                        data.stringDefault = "";
                        data.name = "aiSssSetname";
                        data.shortName = "ai_sss_setname";
                        data.type = AI_TYPE_STRING;
                        helper.MakeInput(data);

                }

                static void *creator()
                {
                        return new BB_gpuCacheTranslator();
                }

        protected :

                void GetDisplacement(MObject& obj,
                                                          float& dispPadding,
                                                          bool& enableAutoBump)
                {
                   MFnDependencyNode dNode(obj);
                   MPlug plug = dNode.findPlug("aiDisplacementPadding");
                   if (!plug.isNull())
                      dispPadding = MAX(dispPadding, plug.asFloat());
                   if (!enableAutoBump)
                   {
                      plug = dNode.findPlug("aiDisplacementAutoBump");
                      if (!plug.isNull())
                         enableAutoBump = enableAutoBump || plug.asBool();
                   }
                }

                /// Returns the arnold shader assigned to the procedural. This duplicates
                /// code in GeometryTranslator.h, but there's not much can be done about that
                /// since the GeometryTranslator isn't part of the MtoA public API.
                AtNode *arnoldShader(AtNode* node)
                {
                  m_displaced = false;

                  float maximumDisplacementPadding = -AI_BIG;
                  bool enableAutoBump = false;

                  unsigned instNumber = m_dagPath.isInstanced() ? m_dagPath.instanceNumber() : 0;
                  MPlug shadingGroupPlug = GetNodeShadingGroup(m_dagPath.node(), instNumber);

                  //find and export any displacment shaders attached
                  // DISPLACEMENT MATERIAL EXPORT
                  MPlugArray        connections;
                  MFnDependencyNode fnDGShadingGroup(shadingGroupPlug.node());
                  MPlug shaderPlug = fnDGShadingGroup.findPlug("displacementShader");
                  shaderPlug.connectedTo(connections, true, false);

                  // are there any connections to displacementShader?
                  if (connections.length() > 0)
                  {
                     m_displaced = true;
                     MObject dispNode = connections[0].node();
                     GetDisplacement(dispNode, maximumDisplacementPadding, enableAutoBump);
                     m_dispPadding = maximumDisplacementPadding;
                     AtNode* dispImage(ExportNode(connections[0]));

                     m_dispNode = dispImage;
                  }

                  // Only export displacement attributes if a displacement is applied
                  if (m_displaced)
                  {
                      std::cout << "arnoldShader::m_displaced :: " << m_displaced << std::endl;
                     // Note that disp_height has no actual influence on the scale of the displacement if it is vector based
                     // it only influences the computation of the displacement bounds
                    // AiNodeSetFlt(node, "disp_padding", maximumDisplacementPadding);
                  }

                  // return the exported surface shader
                  return ExportNode( shadingGroupPlug );
                }

        protected :
                MFnDagNode m_DagNode;
                bool m_isMasterDag;
                bool m_displaced;
                float m_dispPadding;
                MDagPath m_dagPathRef;
                MDagPath m_masterDag;
                AtNode* m_dispNode;
};

extern "C"
{

DLLEXPORT void initializeExtension( CExtension &extension )
{
        extension.Requires( "gpuCache" );
        extension.RegisterTranslator(
                "gpuCache",
                "",
                BB_gpuCacheTranslator::creator,
                BB_gpuCacheTranslator::nodeInitialiser
        );
}

DLLEXPORT void deinitializeExtension( CExtension &extension )
{
}

} // extern "C"





