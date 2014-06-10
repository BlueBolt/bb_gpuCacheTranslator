import pymel.core as pm
import maya.cmds as cmds
import maya.OpenMaya as om
import mtoa.ui.ae.templates as templates
import mtoa.ui.ae.utils as aeUtils
import mtoa.callbacks as callbacks
import mtoa.core as core
import re

def LoadGpuCacheButtonPush(nodeName):
    AttrName = nodeName.split('.')[-1]        
    basicFilter = 'JSON ASCII (*.json)'
    caption = 'Load JSON File'

    if AttrName == "assShaders":
        basicFilter = 'Arnold Archive (*.ass *.ass.gz)'
        caption = 'Load ASS File'

    projectDir = cmds.workspace(query=True, directory=True)     
    ret = cmds.fileDialog2(fileFilter=basicFilter, cap=caption,okc='Load',fm=1, startingDirectory=projectDir)
    if ret is not None and len(ret):
        ArnoldGpuCacheEdit(nodeName, ret[0], True)

def ArnoldGpuCacheEdit(nodeName, mPath, replace=False) :
    AttrName = nodeName.split('.')[-1]        
    cmds.setAttr(nodeName,mPath,type='string')
    cmds.textField('GpuCache%sPath'%AttrName, edit=True, text=mPath)

def ArnoldGpuCacheTemplateNew(plugName) :
    AttrName = plugName.split('.')[-1]
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1 \2', AttrName)
    s2 = re.sub('(.)(file+)', r'\1 \2', s1)
    NiceName = re.sub('([a-z0-9])([A-Z])', r'\1 \2', s2).title()

    cmds.setUITemplate('attributeEditorTemplate',pst=True)
    cmds.rowLayout( numberOfColumns=3 )
    cmds.text(label=NiceName)
    path = cmds.textField('GpuCache%sPath'%AttrName) # ,changeCommand=lambda *args: ArnoldGpuCacheEdit(plugName, *args)
    cmds.symbolButton('GpuCache%sPathButton'%AttrName, image='navButtonBrowse.png')
    cmds.setUITemplate(ppt=True)

    ArnoldGpuCacheTemplateReplace(plugName)

    
def ArnoldGpuCacheTemplateReplace(plugName) :
    AttrName = plugName.split('.')[-1]
    cmds.connectControl('GpuCache%sPath'%AttrName,plugName,fileName=True )

    cmds.textField( 'GpuCache%sPath'%AttrName, edit=True, changeCommand=lambda *args: ArnoldGpuCacheEdit(plugName, *args))
    cmds.textField( 'GpuCache%sPath'%AttrName, edit=True, text=cmds.getAttr(plugName) )
    
    cmds.symbolButton('GpuCache%sPathButton'%AttrName, edit=True, command=lambda *args: LoadGpuCacheButtonPush(plugName))

class GpuCacheTemplate(templates.ShapeTranslatorTemplate):

    def setup(self):
        self.commonShapeAttributes()
        
        self.addSeparator()
        
        self.addControl("objectPattern", label="Object Pattern")
        self.addControl("namePrefix", label="Name Prefix")
        self.addCustom('assShaders', ArnoldGpuCacheTemplateNew, ArnoldGpuCacheTemplateReplace)

        self.beginLayout('Dictionaries', collapse=False)
        self.addControl("shaderAssignation", label="Shaders")
        self.addControl("displacementAssignation", label="Displacements")
        self.addControl("overrides", label="Overrides")
        self.addControl("userAttributes", label="User Attributes")
        self.endLayout()

        self.beginLayout('JSON Files', collapse=False)
        self.addCustom('shaderAssignmentfile', ArnoldGpuCacheTemplateNew, ArnoldGpuCacheTemplateReplace)
        self.addCustom('overridefile', ArnoldGpuCacheTemplateNew, ArnoldGpuCacheTemplateReplace)
        self.addCustom('userAttributesfile', ArnoldGpuCacheTemplateNew, ArnoldGpuCacheTemplateReplace)
        self.endLayout()
        
        self.beginLayout('Skip Options', collapse=False)
        self.addControl("skipJson", label="Skip JSON")
        self.addControl("skipShaders", label="Skip Shaders")
        self.addControl("skipDisplacements", label="Skip Displacements")
        self.addControl("skipOverrides", label="Skip Overrides")
        self.addControl("skipUserAttributes", label="Skip User Attributes")
        self.endLayout()

        self.addSeparator()

        self.beginLayout('Points', collapse=False)
        self.addControl("radiusPoint", label="Point Radius")
        self.addControl("scaleVelocity", label="Scale Velocity")
        self.endLayout()

        self.beginLayout('Advanced', collapse=False)
        self.addControl('makeInstance', label='Make Instance')
        self.addControl('flipv', label='Flip V Coord')
        self.addControl('invertNormals', label='Invert Normals')
        self.endLayout()
        self.addControl("aiUserOptions", label="User Options")


templates.registerAETemplate(GpuCacheTemplate, "gpuCache")