#pragma once


namespace uf {
	class UF_API SoundEmitter {
	public:
		typedef std::vector<uf::Audio> container_t;
	protected:
		uf::SoundEmitter::container_t m_container;
	public:
		~SoundEmitter();
		
		bool has( const std::string& ) const;

		uf::Audio& add();
		uf::Audio& add( const std::string& );
		uf::Audio& load( const std::string& );
		uf::Audio& stream( const std::string& );

		uf::Audio& get( const std::string& );
		const uf::Audio& get( const std::string& ) const;
		
		container_t& get();
		const container_t& get() const;

		void update();
		void cleanup( bool = false );
	};
	class UF_API MappedSoundEmitter {
	public:
		typedef std::unordered_map<std::string, uf::Audio> container_t;
	protected:
		uf::MappedSoundEmitter::container_t m_container;
	public:
		~MappedSoundEmitter();
		
		bool has( const std::string& ) const;

		uf::Audio& add( const std::string& );
		uf::Audio& load( const std::string& );
		uf::Audio& stream( const std::string& );

		uf::Audio& get( const std::string& );
		const uf::Audio& get( const std::string& ) const;
		
		container_t& get();
		const container_t& get() const;

		void update();
		void cleanup( bool = false );
	};
}