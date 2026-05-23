local ent = ent
local scene = entities.currentScene()
local controller = entities.controller()

local timer = Timer.new()
if not timer:running() then
	timer:start();
end

local transform = ent:getComponent("Transform")
local physicsBody = ent:getComponent("PhysicsBody")
local metadata = ent:getComponent("Metadata")


--transform.scale = Vector3f( 5, 5, 5 )
local children = {}
local bodies = {
	torso = { Vector3f(0, 1.0, 0), Vector3f(0.2, 0.3, 0.15), 20.0 },
	head  = { Vector3f(0, 1.6, 0), Vector3f(0.15, 0.15, 0.15), 5.0 },
	
	armLU = { Vector3f(-0.4, 1.2, 0), Vector3f(0.1, 0.2, 0.1), 3.0 },
	armLL = { Vector3f(-0.4, 0.7, 0), Vector3f(0.08, 0.2, 0.08), 2.0 },
	armRU = { Vector3f( 0.4, 1.2, 0), Vector3f(0.1, 0.2, 0.1), 3.0 },
	armRL = { Vector3f( 0.4, 0.7, 0), Vector3f(0.08, 0.2, 0.08), 2.0 },

	legLU = { Vector3f(-0.2, 0.5, 0), Vector3f(0.12, 0.25, 0.12), 6.0 },
	legLL = { Vector3f(-0.2, -0.1, 0), Vector3f(0.1, 0.25, 0.1), 4.0 },
	legRU = { Vector3f( 0.2, 0.5, 0), Vector3f(0.12, 0.25, 0.12), 6.0 },
	legRL = { Vector3f( 0.2, -0.1, 0), Vector3f(0.1, 0.25, 0.1), 4.0 },
}
local constraints = {
	neck = { "torso", "head", Vector3f( 0, 1.4, 0 ), Vector3f( 0, 1, 0 ), math.pi / 4.0, math.pi / 8.0 },

	shoulderL = { "torso", "armLU", Vector3f( -0.3, 1.4, 0 ), Vector3f( -1, 0, 0 ), math.pi / 2.0, math.pi / 4.0 },
	shoulderR = { "torso", "armRU", Vector3f(  0.3, 1.4, 0 ), Vector3f(  1, 0, 0 ), math.pi / 2.0, math.pi / 4.0 },

	elbowL = { "armLU", "armLL", Vector3f( -0.4, 0.95, 0 ), Vector3f( 1, 0, 0 ) },
	elbowR = { "armRU", "armRL", Vector3f(  0.4, 0.95, 0 ), Vector3f( 1, 0, 0 ) },

	hipL = { "torso", "legLU", Vector3f( -0.2, 0.75, 0 ), Vector3f( 0, -1, 0 ), math.pi / 3.0, math.pi / 8.0 },
	hipR = { "torso", "legRU", Vector3f(  0.2, 0.75, 0 ), Vector3f( 0, -1, 0 ), math.pi / 3.0, math.pi / 8.0 },

	kneeL = { "legLU", "legLL", Vector3f( -0.2, 0.2, 0 ), Vector3f( 1, 0, 0 ) },
	kneeR = { "legRU", "legRL", Vector3f(  0.2, 0.2, 0 ), Vector3f( 1, 0, 0 ) },
}

for k, _ in pairs( bodies ) do
	position = bodies[k][1]
	extent = bodies[k][2]
	mass = bodies[k][3]
	
	child = ent:loadChild("./ragdollLimb.json", true)
	childTransform = child:getComponent("Transform")
	childTransform.position = position
	childTransform:setReference( transform )

	body = physics.create( child, mass )
	body:asObb( OBB( Vector3f(), extent ) )
	body:setGravity( Vector3f( 0, 0, 0 ) )

	children[k] = child
	bodies[k] = body
end


for k, _ in pairs( constraints ) do
	bodyA = bodies[ constraints[k][1] ]
	bodyB = bodies[ constraints[k][2] ]

	joint = transform:apply( constraints[k][3] )
	axis = transform.orientation:rotate( constraints[k][4] )

	swingLimit = constraints[k][5]
	twistLimit = constraints[k][6]

	constraint = bodyA:constrain( bodyB )
	if swingLimit == nil or twistLimit == nil then
		constraint:asHinge( joint, axis )
	else
		constraint:asConeTwist( joint, axis, swingLimit, twistLimit )
	end

	constraints[k] = constraint
end