#pragma once


namespace uf {
	class UF_API SoundEmitter {
	public:
		typedef uf::stl::vector<uf::Audio> container_t;
	protected:
		uf::SoundEmitter::container_t m_container;
	public:
		~SoundEmitter();
		
		bool has( const uf::stl::string& ) const;

		uf::Audio& add();
		uf::Audio& add( const uf::stl::string& );
		uf::Audio& load( const uf::stl::string& );
		uf::Audio& stream( const uf::stl::string& );

		uf::Audio& get( const uf::stl::string& );
		const uf::Audio& get( const uf::stl::string& ) const;
		
		container_t& get();
		const container_t& get() const;

		void update();
		void cleanup( bool = false );
	};
	class UF_API MappedSoundEmitter {
	public:
		typedef uf::stl::unordered_map<uf::stl::string, uf::Audio> container_t;
	protected:
		uf::MappedSoundEmitter::container_t m_container;
	public:
		~MappedSoundEmitter();
		
		bool has( const uf::stl::string& ) const;

		uf::Audio& add( const uf::stl::string& );
		uf::Audio& load( const uf::stl::string& );
		uf::Audio& stream( const uf::stl::string& );

		uf::Audio& get( const uf::stl::string& );
		const uf::Audio& get( const uf::stl::string& ) const;
		
		container_t& get();
		const container_t& get() const;

		void update();
		void cleanup( bool = false );
	};
}