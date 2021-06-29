#pragma once

#include <uf/config.h>

#include <cmath>
#include <uf/utils/memory/vector.h>
#include <random>
#include <numeric>
#include <algorithm>
#include <functional>

#include <uf/utils/math/vector.h>

namespace uf {
	class UF_API PerlinNoise {
	protected:
		std::size_t m_seed;
		uf::stl::vector<int> m_permutations;
	public:
		typedef std::function<double(double&)> callback_t;

		PerlinNoise();
		PerlinNoise( std::size_t seed );
		void seed( std::size_t seed );
		std::size_t seed() const;
		double noise( double x, double y, double z ) const;
		double fade(double t) const;
		double lerp(double t, double a, double b) const;
		double grad(int hash, double x, double y, double z) const;
		uf::stl::vector<double> collect( uint size, const pod::Vector3d& noiseCoeff, const callback_t& callback = [](double& n)->double{
			return n;
		} );
		uf::stl::vector<double> collect( const pod::Vector2ui& size, const pod::Vector3d& noiseCoeff, const callback_t& callback = [](double& n)->double{
			return n;
		} );
		uf::stl::vector<double> collect( const pod::Vector3ui& size, const pod::Vector3d& noiseCoeff, const callback_t& callback = [](double& n)->double{
			return n;
		} );

		double sample( const pod::Vector3d& position, const pod::Vector3d& noiseCoeff = { 1, 1, 1 }, const callback_t& callback = [](double& n)->double{
			return n;
		} );
		double sample( const pod::Vector3f& position, const pod::Vector3d& noiseCoeff = { 1, 1, 1 }, const callback_t& callback = [](double& n)->double{
			return n;
		} );
	};
}