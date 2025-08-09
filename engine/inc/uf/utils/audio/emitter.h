#pragma once


namespace uf {
	class UF_API AudioEmitter {
	public:
		typedef uf::stl::vector<uf::Audio> container_t;
	protected:
		uf::AudioEmitter::container_t m_container;
	public:
		~AudioEmitter();
		
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
	class UF_API MappedAudioEmitter {
	public:
		typedef uf::stl::unordered_map<uf::stl::string, uf::Audio> container_t;
	protected:
		uf::MappedAudioEmitter::container_t m_container;
	public:
		~MappedAudioEmitter();
		
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

	using SoundEmitter = AudioEmitter; // alias
	using MappedSoundEmitter = MappedAudioEmitter; // alias
}