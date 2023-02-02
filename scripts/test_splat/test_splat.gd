extends Node3D

@onready var mat = load("res://shaders/splat.material")


var a := Vector3(0,0,0)
var aI := 0b00000000000000000000000000000000
var verts := [a]
var indexVerts := [aI]
# Called when the node enters the scene tree for the first time.
func _ready():
	var mesh := ArrayMesh.new()
	var arrays := []
	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = PackedVector3Array(verts)
	arrays[Mesh.ARRAY_INDEX] = PackedInt32Array(indexVerts)
	
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_POINTS,arrays,[],{},1 << (Mesh.ARRAY_INDEX + 1 + 15))
	var mi := MeshInstance3D.new()

	mesh.surface_set_material(0,mat)
	mi.mesh = mesh;
	mi.set_custom_aabb(AABB(Vector3(0,0,0),Vector3(0.25,0.25,0.25)))
	add_child(mi)
	

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	pass
		
	
