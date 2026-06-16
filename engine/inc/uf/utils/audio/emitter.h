#pragma once

namespace uf {
	class UF_API AudioEmitter {
	public:
		typedef uf::stl::unordered_map<uf::stl::string, uf::stl::vector<pod::AudioSource>> container_t;
	protected:
		container_t m_container;
		
		ext::al::Effect m_effect;
		ext::al::EffectSlot m_effectSlot;
		bool m_efxInitialized = false;
		uf::Timer<> m_acousticTimer = {true};
	public:
		~AudioEmitter();

		bool has( const uf::stl::string& key ) const;

		pod::AudioSource& emit( const uf::stl::string& key, pod::AudioClip* clip, bool unique = false );

		pod::AudioSource& get( const uf::stl::string& key );
		const pod::AudioSource& get( const uf::stl::string& key ) const;

		container_t& get();
		const container_t& get() const;

		void update();
		void update( const pod::Vector3f& position, const pod::Quaternion<>& orientation );
		void updateAcoustics( const pod::Vector3f& position, const pod::Quaternion<>& orientation );
		void cleanup( bool purge = false );
	};

	using MappedAudioEmitter 	= AudioEmitter;
	using SoundEmitter	   		= AudioEmitter;
	using MappedSoundEmitter 	= AudioEmitter;
}