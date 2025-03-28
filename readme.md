# Describe the scene with a XML file
````xml
<?xml version="1.0" encoding="utf-8"?>
<!-- Describe your camera -->
<camera type="perspective" width="1280" height="720" fovy="35.9834">
	<eye x="4.443147659301758" y="16.934431076049805" z="49.91023254394531"/> 
	<lookat x="-2.5734899044036865" y="9.991769790649414" z="-10.588199615478516"/> 
	<up x="0.0" y="1.0" z="0.0"/> 
</camera>


<!-- Describe your scene -->
<scene>
    <object>
        <filepath objpath="./demo.obj"/>
        <transform>
            <translation x="1" y="1" z="1">
            <rotation radians="0" axis="(0,0,1)">
            <scale factor="(10,10,10)">
        </transform>

        <flag name="filpnorm" value="true"/>

        <material name="diff0">

    </object>


</scene>

<!-- Describe your bsdf wrt material -->
<bsdf mtlname="diff0" bsdftype="lambert">

<!-- Describe your Light Material(only support Area Light now) -->
<light mtlname="Light" radiance="125.0,100.0,75.0"/>

````
