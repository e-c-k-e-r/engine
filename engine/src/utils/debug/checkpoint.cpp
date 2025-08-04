#include <uf/utils/debug/checkpoint.h>
#include <uf/utils/memory/pool.h>
#include <uf/utils/memory/allocator.h>
#include <uf/utils/string/ext.h>

pod::Checkpoint* uf::checkpoint::allocate( const pod::Checkpoint& checkpoint ) {
	pod::Checkpoint* pointer = new pod::Checkpoint;// (pod::Checkpoint*) uf::allocator::allocate( sizeof(pod::Checkpoint) );
	pointer->type = checkpoint.type;
	pointer->name = checkpoint.name;
	pointer->info = checkpoint.info;
	pointer->previous = checkpoint.previous;
	return pointer;
}
pod::Checkpoint* uf::checkpoint::allocate( pod::Checkpoint::Type type, const uf::stl::string& name, const uf::stl::string& info, pod::Checkpoint* previous ) {
	pod::Checkpoint* pointer = new pod::Checkpoint;// (pod::Checkpoint*) uf::allocator::allocate( sizeof(pod::Checkpoint) );
	pointer->type = type;
	pointer->name = name;
	pointer->info = info;
	pointer->previous = previous;
	return pointer;
}
void uf::checkpoint::deallocate( pod::Checkpoint& checkpoint ) { return deallocate( &checkpoint );}
void uf::checkpoint::deallocate( pod::Checkpoint* checkpoint ) {
	if ( !checkpoint ) return;
	deallocate( checkpoint->previous );
	delete checkpoint;
/*
	pod::Checkpoint* previous = NULL;
	while ( checkpoint ) {
		previous = checkpoint->previous;
		checkpoint->name = "";
		checkpoint->info = "";
		checkpoint->previous = NULL;
	//	uf::allocator::deallocate( checkpoint );
		delete checkpoint;
		checkpoint = previous;
	}
*/
}
uf::stl::string uf::checkpoint::traverse( pod::Checkpoint& checkpoint ) {
	return traverse( &checkpoint );
}
uf::stl::string uf::checkpoint::traverse( pod::Checkpoint* checkpoint ) {
	uf::stl::vector<uf::stl::string> res;

	while ( checkpoint ) {
		uf::stl::string type = "";
		switch ( checkpoint->type ) {
			case pod::Checkpoint::GENERIC: type = "GENERIC"; break;
			case pod::Checkpoint::BEGIN: type = "BEGIN"; break;
			case pod::Checkpoint::END: type = "END"; break;
		}
	#if UF_USE_FMT
		res.emplace( res.begin(), ::fmt::format("[{}] [{}]: {}", type, checkpoint->info, checkpoint->name) );
	#else
		res.emplace( res.begin(), "[" + type + "] [" + checkpoint->info + "]: " + checkpoint->name );
	#endif
		checkpoint = checkpoint->previous;
	}	

	return uf::string::join( res, "\n" );
}