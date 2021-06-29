#include <uf/utils/math/collision.h>
#include <uf/utils/math/rayt.h>
#include <iostream>

// compile-time assert to ensure objects are of same size, if using multiple types of objects

// takes a vector of primitives and groups them under leaves (end point of a tree)
uf::stl::vector<pod::Tree> UF_API uf::primitive::populate( const uf::stl::vector<pod::Primitive>& cubes ) { // assert(cubes.size() > 0);
	uf::stl::vector<pod::Tree> trees;
	uf::stl::vector<const pod::Primitive*> copy;
	uf::stl::vector<const pod::Primitive*> alloced;
	copy.reserve( cubes.size() );
//	std::cout << "Size: " << cubes.size() << std::endl;
	for ( const auto& cube : cubes ) {
		copy.push_back( &cube );
	}

	trees.reserve( copy.size() / pod::Tree::TREE_SIZE );
	alloced.reserve( copy.size() );
	const float cubeRoot = std::cbrt(pod::Tree::TREE_SIZE);
	for ( const auto& cube : cubes ) {
		// already allocated, skip
		if ( std::find( alloced.begin(), alloced.end(), &cube ) != alloced.end() ) continue;
		size_t index = 0;
		pod::Tree tree;
		// sort by closest cube
		std::sort( copy.begin(), copy.end(), [&]( const pod::Primitive* l, const pod::Primitive* r ) {
			return uf::vector::distanceSquared( cube.position, l->position ) < uf::vector::distanceSquared( cube.position, r->position );
		} );
//		std::cout << trees.size() << ": " << alloced.size() << std::endl;
		while ( index < pod::Tree::TREE_SIZE && copy.begin() != copy.end() ) {
			const pod::Primitive* ptr = *copy.begin();
			copy.erase( copy.begin() );
			tree.children[index++] = (ptr - &cubes[0]);
		}
		for ( size_t i = index; i < pod::Tree::TREE_SIZE; ++i ) {
			tree.children[i] = pod::Primitive::EMPTY;
		}
		// mark children as allocated
	/*
		for ( int i = 0; i < pod::Tree::TREE_SIZE && i < index; ++i ) {
			const auto& cube = cubes.at(tree.children[i]);
			alloced.push_back(&cube);
			tree.primitive.position.x += cube.position.x; tree.primitive.position.y += cube.position.y; tree.primitive.position.z += cube.position.z;
		}
		tree.primitive.position.x /= pod::Tree::TREE_SIZE;
		tree.primitive.position.y /= pod::Tree::TREE_SIZE;
		tree.primitive.position.z /= pod::Tree::TREE_SIZE;
	*/
		pod::Vector3f min = { 0, 0, 0 }, max = { 0, 0, 0 };
		for ( int i = 0; i < pod::Tree::TREE_SIZE && i < index; ++i ) {
			const auto& cube = cubes.at(tree.children[i]);
			alloced.push_back(&cube);
			if ( i == 0 ) {
				min.x = cube.position.x - cube.position.w;
				min.y = cube.position.y - cube.position.w;
				min.z = cube.position.z - cube.position.w;
				max.x = cube.position.x + cube.position.w;
				max.y = cube.position.y + cube.position.w;
				max.z = cube.position.z + cube.position.w;
				continue;
			}
			min.x = std::min( cube.position.x - cube.position.w, min.x ); min.y = std::min( cube.position.y - cube.position.w, min.y ); min.z = std::min( cube.position.z - cube.position.w, min.z );
			max.x = std::max( cube.position.x + cube.position.w, max.x ); max.y = std::max( cube.position.y + cube.position.w, max.y ); max.z = std::max( cube.position.z + cube.position.w, max.z );
		}
		tree.position.x = (max.x + min.x) * 0.5f;
		tree.position.y = (max.y + min.y) * 0.5f;
		tree.position.z = (max.z + min.z) * 0.5f;
	
		tree.position.w = std::max( tree.position.w, (max.x - min.x) * 0.5f );
		tree.position.w = std::max( tree.position.w, (max.y - min.y) * 0.5f );
		tree.position.w = std::max( tree.position.w, (max.z - min.z) * 0.5f );
	//	tree.position.w = 0.5;
		tree.type = pod::Primitive::LEAF; // leaves point to cubes
		trees.push_back(tree);
	}
	return trees;
}

// takes a list of leaves (ends of trees) and properly populate an entire tree
uf::stl::vector<pod::Tree> UF_API uf::primitive::populateEntirely( const uf::stl::vector<pod::Tree>& trees, bool rooted ) { // assert(trees.size() > 0);
	// generate first layer
	uf::stl::vector<pod::Tree> tree = trees;
	uf::stl::vector<uint32_t> referred;
	uf::stl::vector<uint32_t> queue;
	uint32_t iteration = 0;
	uint32_t start = 0;
/*
	std::cout << "Primitives:\n\t";
	for ( auto& prim : primitives ) {
		size_t i = &prim - &primitives[0];
		std::cout << (i < 10 ? "0" : "" ) <<  i << " ";
		if ( (i+1) % 8 == 0 ) std::cout << "\n\t";
	}
	std::cout << std::endl;
*/
loop: {
//	std::cout << "Iteration #" << ++iteration << std::endl;
	referred.clear();
	// grab all unreferenced trees
	for ( auto& branch : tree ) {
		if ( branch.type != pod::Primitive::TREE ) continue;
		referred.insert( referred.end(), &branch.children[0], &branch.children[7] );
//		std::cout << "Tree #" << (&branch - &tree[0]) << " refers to:\n";
		for ( uint i = 0; i < pod::Tree::TREE_SIZE; ++i ) {
//			std::cout << "\t" << branch.children[i] << " ";
			referred.push_back(branch.children[i]);
		}
//		std::cout << std::endl;
	}
	queue.clear();
	// check if referred
//	std::cout << "Queueing:\n\t";
	for ( auto& branch : tree ) {
		size_t i = &branch - &tree[0];
		if ( std::find( referred.begin(), referred.end(), i ) != referred.end() ) continue;
//		std::cout << i << " ";
		queue.push_back( &branch - &tree[0] );
	}
//	std::cout << std::endl;
	// grab the branches' primitives
	uf::stl::vector<pod::Primitive> enqueued; enqueued.reserve( queue.size() );
	for ( uint32_t i : queue ) {
		pod::Tree t = tree.at(i);
		pod::Primitive primitive;
		primitive.position = t.position;
		primitive.position.w = t.position.w;
		primitive.type = i;
		enqueued.push_back( primitive );
	}
	// create a parent tree
	uf::stl::vector<pod::Tree> newTree = populate( enqueued );
	// convert indices and type
	for ( auto& branch : newTree ) {
		branch.type = pod::Primitive::TREE;
		branch.position.w = (iteration+1.0f) * 0.5f;
		for ( size_t i = 0; i < pod::Tree::TREE_SIZE; ++i ) {
			size_t index = branch.children[i];
			if ( index == pod::Primitive::EMPTY ) continue;
			branch.children[i] = enqueued.at(index).type;
		}
	}
	// copy new tree to beginning of tree list
	tree.insert( tree.end(), newTree.begin(), newTree.end() );
	start = tree.size();
	// cannot divide furter
	if ( newTree.size() <= 1 ) goto finished;
}
	goto loop;
finished:
	if ( rooted ) tree.at(tree.size()-1).type = pod::Primitive::ROOT;

//	std::cout << "Full tree:\n";
	for ( auto& branch : tree ) {
//		std::cout << "\t" << (&branch - &tree[0]) << " (type: " << (branch.type) << ") has children:\n\t\t";
		for ( size_t i = 0; i < pod::Tree::TREE_SIZE; ++i ) {
//			std::cout << branch.children[i] << " ";
		}
//		std::cout << std::endl;
 	}
//	std::cout << std::endl;
	for ( auto& branch : tree ) {
//		std::cout << (&branch - &tree[0]) << " (type: " << (branch.type) << ") primitive:\n";
//		std::cout << "\tPosition: " << branch.position.x << ", " << branch.position.y << ", " << branch.position.z << "\n\tSize: " << branch.position.w << "\n\t\t";
		for ( size_t i = 0; i < pod::Tree::TREE_SIZE; ++i ) {
//			std::cout << branch.children[i] << " ";
		}
//		std::cout << std::endl;
		/*
		if ( branch.type == pod::Primitive::LEAF ) {
			for ( size_t i = 0; i < pod::Tree::TREE_SIZE; ++i ) {
				if ( branch.children[i] == pod::Primitive::EMPTY ) continue;
				auto& pr = primitives.at(branch.children[i]);
//				std::cout << "\t\tPosition: " << pr.position.x << ", " << pr.position.y << ", " << pr.position.z << "\n\t\tSize: " << pr.position.w << std::endl;
			}
		} else {
			for ( size_t i = 0; i < pod::Tree::TREE_SIZE; ++i ) {
				if ( branch.children[i] == pod::Primitive::EMPTY ) continue;
				auto& pr = tree.at(branch.children[i]);
//				std::cout << "\t\tPosition: " << pr.position.x << ", " << pr.position.y << ", " << pr.position.z << "\n\t\tSize: " << pr.position.w << std::endl;
			}
		}
		*/
 	}
//	std::cout << std::endl;

	return tree;
}












////////////////////////
namespace {
	#define EPSILON 0.0001f
	#define MAXLEN 1000.0f
	#define SHADOW 0.5f

	#define TREE_SIZE 8
	#define TREE_STACK 8
	#define PRIMITIVE_TYPE_EMPTY UINT32_MAX
	#define PRIMITIVE_TYPE_CUBE 1
	#define PRIMITIVE_TYPE_LEAF 2
	#define PRIMITIVE_TYPE_TREE 3
	#define PRIMITIVE_TYPE_ROOT 4
	struct StackIterator {
		uint tree;
		uint child;
	};
	struct Stack {
		int pointer;
		StackIterator container[TREE_STACK];
	} stack;
	void printStack( ) {
		std::cout << "[*] Pointer @ " << stack.pointer << std::endl;
		for ( int i = stack.pointer; i >= 0; --i ) { 
			std::cout << "[*] " << stack.container[i].tree << ", " << stack.container[i].child << std::endl;
		}
		std::cout << std::endl;
	}
	StackIterator popStack( ) {
		StackIterator top = stack.container[stack.pointer];
		stack.container[stack.pointer--] = { UINT32_MAX, UINT32_MAX };
		std::cout << "[?] popped stack" << std::endl;
		printStack();
		return top;
	}
	StackIterator topStack( ) {
		StackIterator pointer = stack.container[stack.pointer];
		printStack();
		return pointer;
	}
	void pushStack( StackIterator item ) {
		stack.container[++stack.pointer] = item;
		std::cout << "[?] pushed to stack" << std::endl;
		printStack();
	}
	float cubeIntersect( const pod::Vector3f& rayO, const pod::Vector3f& rayD, const pod::Vector3f& rayDRecip, const pod::Primitive& cube) {
		float t[10];
		t[1] = ( cube.position.x - cube.position.w - rayO.x) / rayD.x;
		t[2] = ( cube.position.x + cube.position.w - rayO.x) / rayD.x;
		t[3] = ( cube.position.y - cube.position.w - rayO.y) / rayD.y;
		t[4] = ( cube.position.y + cube.position.w - rayO.y) / rayD.y;
		t[5] = ( cube.position.z - cube.position.w - rayO.z) / rayD.z;
		t[6] = ( cube.position.z + cube.position.w - rayO.z) / rayD.z;
		t[7] = std::max(std::max(std::min(t[1], t[2]), std::min(t[3], t[4])), std::min(t[5], t[6]));
		t[8] = std::min(std::min(std::max(t[1], t[2]), std::max(t[3], t[4])), std::max(t[5], t[6]));
		t[9] = (t[8] < 0 || t[7] > t[8]) ? 0.0 : t[7];
		return t[9];
	}
	float treeIntersect( const pod::Vector3f& rayO, const pod::Vector3f& rayD, const pod::Vector3f& rayDRecip, const pod::Tree& tree ) {
		pod::Primitive treecube;
		treecube.type = tree.type;
		treecube.position = tree.position;
		return cubeIntersect( rayO, rayD, rayDRecip, treecube );
	}
}

void uf::primitive::test( const uf::stl::vector<pod::Primitive>& cubes, const uf::stl::vector<pod::Tree>& trees ) {
	pod::Vector3f rayO = { 0, 5, 0 };
	pod::Vector3f rayD = uf::vector::normalize( pod::Vector3f{ 0, -1, -0.25 } );
	pod::Vector3f rayDRecip = { 1.0f / rayD.x, 1.0f / rayD.y, 1.0f / rayD.z };
	float resT = MAXLEN;
	uint root = trees.size() - 1;
	uint id = UINT32_MAX;
	if ( root == UINT32_MAX ) return;

	// set up stack
	stack.pointer = -1;
	for ( uint i = 0; i < TREE_STACK; ++i ) stack.container[i] = {UINT32_MAX, UINT32_MAX};
	pushStack({ root, UINT32_MAX });

	while ( true ) {
		StackIterator it = popStack();
		// end of stack
		if ( it.tree == UINT32_MAX ) break;
		pod::Tree tree = trees[it.tree];
		// invalid tree
		if ( tree.type == PRIMITIVE_TYPE_EMPTY ) break;
		// new tree, parse collision
		if ( it.child == UINT32_MAX ) {
			float t = treeIntersect( rayO, rayD, rayDRecip, tree );
			// bad intersection with this tree, continue with next iteration
			std::cout << "[?] Collision: 0 < " << t << " < " << resT << std::endl;
			if ( t <= EPSILON || t >= resT ) continue;
			// push back with new stack
			it.child = 0;
			pushStack( it );
			// continue with next iteration
			continue;
		} else if ( it.child >= TREE_SIZE ) {
			// no new children, continue with next iteration
			continue;
		} else {
			// is leaf
			if ( tree.type == PRIMITIVE_TYPE_LEAF ) {
				// check children for a match
				for ( uint i = 0; i < TREE_SIZE; ++i ) {
					uint branchId = tree.children[i];
					// unallocated, skip
					if ( branchId == UINT32_MAX ) continue;
					pod::Primitive primitive = cubes[branchId];
					if ( primitive.type == PRIMITIVE_TYPE_EMPTY ) continue;
					float t = cubeIntersect( rayO, rayD, rayDRecip, primitive );
					// branch intersects with ray, set as new parent
					if ( (t <= EPSILON) || (t >= resT) ) continue;
					id = branchId;
					resT = t;
				}
				// continue with next iteration
				continue;
			}
			// parse children
			uint branchId = tree.children[it.child++];
			// add new iterator to the stack
			pushStack( it );
			// unused child, continue with next iteration
			if ( branchId == UINT32_MAX ) continue;
			// tree branch, push to stack
			// the first if block will check its collision
			it.tree = branchId;
			it.child = UINT32_MAX;
			pushStack( it );
			continue;
		}
	}
	std::cout << "ID: " << id << std::endl;
}