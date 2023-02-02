extends Node3D

@onready var mat = load("res://shaders/test.material")

# I like to use this convention, but you can use whatever you like
#               010           110                         Y
#   Vertices     A0 ---------- B1            Faces      Top    -Z
#           011 /  |      111 /  |                        |   North
#             E4 ---------- F5   |                        | /
#             |    |        |    |          -X West ----- 0 ----- East X
#             |   D3 -------|-- C2                      / |
#             |  /  000     |  / 100               South  |
#             H7 ---------- G6                      Z    Bottom
#              001           101                          -Y

var a := Vector3(0, 1, 0) # if you want the cube centered on grid points
var b := Vector3(1, 1, 0) # you can subtract a Vector3(0.5, 0.5, 0.5)
var c := Vector3(1, 0, 0) # from each of these
var d := Vector3(0, 0, 0)
var e := Vector3(0, 1, 1)
var f := Vector3(1, 1, 1)
var g := Vector3(1, 0, 1)
var h := Vector3(0, 0, 1)

var aI := 0b00000000000000000000000100000000
var bI := 0b00000000000000000000000100000001
var cI := 0b00000000000000000000000000000001
var dI := 0b00000000000000000000000000000000
var eI := 0b00000000000000010000000100000000
var fI := 0b00000000000000010000000100000001
var gI := 0b00000000000000010000000000000001
var hI := 0b00000000000000010000000000000000

#var vertices := [   # faces (triangles)
#	b,a,d,  b,d,c,  # N
#	e,f,g,  e,g,h,  # S
#	a,e,h,  a,h,d,  # W
#	f,b,c,  f,c,g,  # E
#	a,b,f,  a,f,e,  # T
#	h,g,c,  h,c,d,  # B
#]
var vertices := [
	f
]

var indexEncodeVerts := [
	bI,aI,dI,  bI,dI,cI,  # N
	eI,fI,gI,  eI,gI,hI,  # S
	aI,eI,hI,  aI,hI,dI,  # W
	fI,bI,cI,  fI,cI,gI,  # E
	aI,bI,fI,  aI,fI,eI,  # T
	hI,gI,cI,  hI,cI,dI,  # B
]

# Called when the node enters the scene tree for the first time.
func _ready():
	var mesh := ArrayMesh.new()
	var dict = mesh.get_property_list()
	for i in dict:
		print("{key}".format({"key":i}))
	var arrays := []
	arrays.resize(Mesh.ARRAY_MAX)
#	arrays[Mesh.ARRAY_VERTEX] = PackedVector3Array(vertices)
	arrays[Mesh.ARRAY_INDEX] = PackedInt32Array(indexEncodeVerts)

	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES,arrays,[],{},1 << (Mesh.ARRAY_INDEX + 1 + 15))
	print(mesh.surface_get_format(0))

	var mi := MeshInstance3D.new()
	
	mesh.surface_set_material(0,mat)
	mi.mesh = mesh
	add_child(mi)
	
#	var shiftVal = gI >> 8 & 0b1111
#	var num=3
#	num += ++num + ++num
#	print("Bit shift value {bitVal}, surface count {surfcount}, number test {num}".format({"bitVal":shiftVal,"surfcount":mesh.get_surface_count(),"num":num}))
	var ref:CalculatorNode = CalculatorNode.new()
	print(ref)
#	var goxalpRef = GoxlapSpatial.new()
#	goxalpRef.set_override_shader(mat.get_rid())
#	goxalpRef.set_renderingserver_props(get_world_3d().scenario);
#	goxalpRef._ready()
#	self.add_child(goxalpRef)

#	var rsInstance = RenderingServer.instance_create()
#	var scenario = get_world_3d().scenario
#	var meshRID = RenderingServer.mesh_create()
#	RenderingServer.instance_set_scenario(rsInstance,scenario)
#	RenderingServer.mesh_add_surface_from_arrays(meshRID,RenderingServer.PRIMITIVE_TRIANGLES,arrays,[],{},1 << (Mesh.ARRAY_INDEX + 1 + 15))
#	RenderingServer.mesh_surface_set_material(meshRID,0,mat.get_rid())
#	var aabb = AABB(Vector3(0,0,0),Vector3(1,1,1))
#	RenderingServer.mesh_set_custom_aabb(meshRID,aabb);
#	RenderingServer.instance_set_base(rsInstance,meshRID)
	
	
# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	pass
