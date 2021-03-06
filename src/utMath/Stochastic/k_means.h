/*
 * Ubitrack - Library for Ubiquitous Tracking
 * Copyright 2006, Technische Universitaet Muenchen, and individual
 * contributors as indicated by the @authors tag. See the 
 * copyright.txt in the distribution for a full listing of individual
 * contributors.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA, or see the FSF site: http://www.fsf.org.
 */

 /**
 * @ingroup math stochastic
 * @file
 *
 * Several functions necessary for k-means calculations including the k-means algorithm.
 *
 * @author Christian Waechter <christian.waechter@in.tum.de>
 */ 

#ifndef __UBITRACK_MATH_STOCHASTIC_K_MEANS_H__
#define __UBITRACK_MATH_STOCHASTIC_K_MEANS_H__
 

// Ubitrack
#include "../Vector.h"
#include "../Blas1.h" // InnnerProduct
#include "../VectorFunctions.h" // Distance
#include "../Random/Scalar.h" // Randomness needed for kmeans++

// some helper header/template stuff
#include <utUtil/StaticAssert.h>
#include "../Geometry/container_traits.h"
#include "identity_iterator.h"


// std
#include <vector>
#include <limits> 
#include <numeric>
#include <algorithm>
#include <functional>

namespace {

// small internal template to support k-means: 
// assigns the indices, according to the minimal distance between
// cluster and vector. 
// -> index relates to number of nearest cluster
template< typename InputIterator, typename BinaryOperator  >
struct assign_indices
{
	typedef typename std::iterator_traits< InputIterator >::value_type vector_type;
	typedef typename vector_type::value_type value_type;
	const InputIterator iBegin;
	const InputIterator iEnd;
	BinaryOperator distanceFunc;
	// const std::size_t n;
	// std::vector< value_type > distances;
	// const typename std::vector< value_type >::iterator itD;
	
	
	assign_indices( const InputIterator first , const InputIterator last, BinaryOperator distanceFuncIn )
		: iBegin( first )
		, iEnd( last )
		, distanceFunc( distanceFuncIn )
		// , n( std::distance( first, last ) )
		// , distances( n, 0 )
		// , itD ( distances.begin() )
		{}
	

	template< template< typename, std::size_t > class VecType, typename T, std::size_t N >
	std::size_t operator()( const VecType< T, N >& vec ) const
	{
		// std::vector< T > distances;
		// distances.reserve( n );
		// std::transform( iBegin, iEnd, Ubitrack::Util::identity< VecType< T, N > >( vec ).begin(), std::back_inserter( distances ), SquarredDistance() );
		// std::transform( iBegin, iEnd, Ubitrack::Util::identity< VecType< T, N > >( vec ).begin(), itD, SquarredDistance() );
		// const std::vector< value_type >::iterator itDist = std::min_element( itD, distances.end() );
		// return std::distance( itD, itDist );
		
		std::size_t k( 0 );
		InputIterator iter = iBegin;
		value_type d = distanceFunc( vec, (*iter++) );
		
		for( std::size_t k_now( 1 ) ; iter != iEnd; ++iter, ++k_now )
		{
			const value_type d_new = distanceFunc( vec, (*iter) );
			if( d_new < d )
			{
				d = d_new;
				k = k_now;
			}
		}
		return k;
	}
};

// small template to support k-means
template< typename InputIterator >
struct assign_to_mean
{
	InputIterator iBegin;
	
	assign_to_mean( InputIterator first )
		: iBegin( first ) {}

	template< typename VecType, typename iType >
	void operator()( const VecType& vec, const iType index )
	{
		*(iBegin + index) += vec;
	}
};

// a helper function
template< typename T1, typename T2, typename BinaryFunction >
BinaryFunction k_means_accumulate( const T1 pFirst1, const T1 pLast, const T2 pFirst2, BinaryFunction op )
{
	T1 first1 = pFirst1;
	T2 first2 = pFirst2;
	for( ; first1 != pLast; ++first1, ++first2 )
		op( *first1, *first2 );

	return op;
};

} // namespace anonymous


namespace Ubitrack{ namespace Math { namespace Stochastic {
 
/**
 * @ingroup math stochastic
 * @brief picks the first k elements of a given range.
 *
 * This function can be applied to nearly any container structure
 * providing access to the single container elements via input iterators.
 * Although designed for stl-containers * (e.g. \c std::vector,
 * \c std::list or \c std::set, etc ) the algorithm is not limited
 * to these ones but can be used via any pointer to data-structures.
 *
 * Example use case:\n
 * std::vector< Vector3d > points3d; // <- class to pick some \c k elements \n
 * std::vector< Vector3d > points3dOut; // <- will be filled with values, storage can be allocated with \c reserve() \n
 * copy_greedy( points3d.begin(), points3d.end(), k, std::back_inserter( points3dOut ) );\n
 * 
 * @tparam InputIterator type of input iterator to container of values
 * @tparam OutputIterator type of the output iterator to container for selected input elements
 * @param iBegin \c iterator pointing to first element in the input container/storage class of the elements ( usually \c begin() )
 * @param iEnd \c iterator pointing behind the last element in the input container/storage class of the elements ( usually \c end() )
 * @param n_cluster a value that signs how many elements should be selected and put to the output iterator
 * @param itSelected output \c iterator pointing to first element in container/storage class for storing the selected elements ( usually \c begin() or \c std::back_inserter(container) )
 */ 
template< typename InputIterator, typename OutputIterator >
void copy_greedy( const InputIterator iBegin, const InputIterator iEnd, const std::size_t n_cluster, OutputIterator itSelected )
{
	typedef typename std::iterator_traits< InputIterator >::value_type vector_type;
	const InputIterator itFinal = iBegin + n_cluster;
	for( InputIterator iter = iBegin;  iter<itFinal ; ++iter )
		(*itSelected++) = ( *iter );
};

/**
 * @ingroup math stochastic
 * @brief picks the first k elements of a given range, depending
 * on the distance of the elements and their probability.
 *
 * This function is also known to be applied for the k-mean++ 
 * algorithm to find good initial values for the centroids.
 *
 * This function can be applied to nearly any container structure
 * providing access to the single container elements via input iterators.
 * Although designed for stl-containers * (e.g. \c std::vector,
 * \c std::list or \c std::set, etc ) the algorithm is not limited
 * to these ones but can be used via any pointer to data-structures.
 * 
 *
 * Example use case:\n
 * std::vector< Vector3d > points3d; // <- class to pick some \c k elements \n
 * std::vector< Vector3d > points3dOut; // <- will be filled with values, storage can be allocated with \c reserve() \n
 * copy_greedy( points3d.begin(), points3d.end(), k, std::back_inserter( points3dOut ) );\n
 * 
 * @tparam InputIterator type of input iterator to container of input elements
 * @tparam OutputIterator type of the output iterator to container for selected input elements
 * @param iBegin \c iterator pointing to first element in the input container/storage class of the elements ( usually \c begin() )
 * @param iEnd \c iterator pointing behind the last element in the input container/storage class of the elements ( usually \c end() )
 * @param n_cluster a value that signs how many elements should be selected and put to the output iterator
 * @param itSelected output \c iterator pointing to first element in container/storage class for storing the selected elements ( usually \c begin() or \c std::back_inserter(container) )
 * @return number of detected intial values, can be different (lower) than desired amount of clusters.
 */ 
template< typename InputIterator, typename OutputIterator, typename BinaryOperator >
std::size_t copy_probability( const InputIterator iBegin, const InputIterator iEnd, const std::size_t n_cluster, OutputIterator itSelected, BinaryOperator distanceFunc )
{
	typedef typename std::iterator_traits< InputIterator >::value_type vector_type;
	typedef typename vector_type::value_type value_type;
	typedef typename std::size_t size_type;

	const std::size_t n = std::distance( iBegin, iEnd );
	std::size_t index = Math::Random::distribute_uniform< std::size_t >( 0, n-1 );
	
	// assign first selected element
	InputIterator itNewOut = (iBegin+index);
	*itSelected++ = *(itNewOut);

	// calculate distances to first element
	std::vector< value_type > distances;
	distances.reserve( n );
	std::transform( iBegin, iEnd, Ubitrack::Util::identity< vector_type >( *itNewOut ).begin(), std::back_inserter( distances ), distanceFunc );

	value_type dist_sum = std::accumulate( distances.begin(), distances.end(), static_cast< value_type >( 0 ) );

	for( size_type k( 1 ); k < n_cluster; ++k, ++itSelected )
	{
		if( dist_sum<= 0 )
			return k;

		value_type max_range = Math::Random::distribute_uniform< value_type >( 0, dist_sum );

		for( index = 0; index < (n-1); ++index )
			if ( max_range <= distances[ index ] )
				break;
			else
				max_range -= distances[ index ];

		// found new value, add it to output
		itNewOut = iBegin+index;
		(*itSelected) = *(itNewOut);
		
		// calculate the distances to the new value
		std::vector< value_type > distances_temp;
		distances_temp.reserve( n );
		std::transform( iBegin, iEnd, Ubitrack::Util::identity< vector_type >( *itNewOut ).begin(), std::back_inserter( distances_temp ), distanceFunc );

		// determine the minimal distance to one of earlier chosen points
		std::transform( distances.begin(), distances.end(), distances_temp.begin(), distances.begin(),
				#ifdef COMPILER_USE_CXX11
				(const value_type& (*)(const value_type&, const value_type&))std::min< value_type >
				#else
				std::min< value_type >
				#endif
		);
		// calculate newest maximal distance (should be smaller than before)
		dist_sum = std::accumulate( distances.begin(), distances.end(), static_cast< value_type >( 0 ) );
	}
	return n_cluster;
};


/** @internal function only, please see other k-means for explanation
 *
 *
 * @tparam InputIterator type of input iterator to container of values
 * @tparam OutputIterator type of the output iterator to container of the clusters
 * @tparam IndicesIterator type of the output iterator to container of the indices
 * @tparam BinaryOperator type of the evaluation function to estimate distance of elements
 */
 
template< typename InputIterator, typename OutputIterator, typename IndicesIterator, typename BinaryOperator >
typename std::iterator_traits< OutputIterator >::value_type::value_type k_means( 
	const InputIterator iBegin, const InputIterator iEnd
	, OutputIterator itMeanBegin, OutputIterator itMeanEnd
	, IndicesIterator indicesOut, BinaryOperator distanceFunc )
{
	// some typedefs regarding the input vector type
	typedef typename std::iterator_traits< InputIterator >::value_type vector_in_type;
	
	// some typedefs regarding the output iterator container (including the means)
	typedef typename std::iterator_traits< OutputIterator >::value_type vector_out_type;
	typedef typename std::vector< vector_out_type > mean_container_type;
	typedef typename mean_container_type::iterator mean_type_iterator;
	typedef typename vector_out_type::value_type value_type; // <- the means define the distance type
	
	// some typedefs regarding the indices container
	typedef typename Ubitrack::Util::container_traits< IndicesIterator >::value_type indices_type;
	typedef typename std::vector< indices_type > indices_container_type;

	// we use a quadratic type for epsilon, such that we do not need to calculate square roots later
	const value_type epsilon = std::pow( static_cast< value_type > (1e-02), 2 );
	const std::size_t max_iter = 100; // I hope the name says all
	
	// std::cout << "Vector Type " << typeid( vector_in_type ).name() << " of dimension=" << N << " and type=" << typeid( T ).name() << "\n";	
	
	const std::size_t n = std::distance( iBegin, iEnd );
	const std::size_t n_cluster = std::distance( itMeanBegin, itMeanEnd );
	assert( n > n_cluster );
	// std::cout << "k-means called with " << n << " elements to determine " << n_cluster << " clusters\n";

	//assign indices for the first time
	indices_container_type indices;
	indices.reserve( n );
	std::transform( iBegin, iEnd, std::back_inserter( indices ), assign_indices< mean_type_iterator, BinaryOperator >( itMeanBegin, itMeanEnd, distanceFunc ) );
	
	
	std::size_t i( 0 );
	value_type diff_error;
	for( ; i<max_iter; ++i )
	{
		// set some "fresh" (empty) mean values 
		mean_container_type means_temp;
		means_temp.reserve( n_cluster );
		means_temp.assign( n_cluster, vector_out_type::zeros() );
				
		//accumulate the means from the clusters 
		k_means_accumulate( iBegin, iEnd, indices.begin(), assign_to_mean< mean_type_iterator >( means_temp.begin() ) );
		
		// reset the amount of gathered values for the means, could be done nicer...
		for( std::size_t k = 0; k<n_cluster; ++k )
			means_temp[ k ] /= std::count ( indices.begin(), indices.end(), k );
		
		// calculate the summarized difference
		std::vector< value_type > norms1;
		norms1.reserve( n_cluster );
		std::transform( itMeanBegin, itMeanEnd, means_temp.begin(), std::back_inserter( norms1 ), distanceFunc );
		diff_error = std::accumulate( norms1.begin(), norms1.end(), static_cast< value_type >( 0 ) ) / n_cluster;
		
		// store the new means values
		itMeanEnd = std::copy( means_temp.begin(), means_temp.end(), itMeanBegin );

		// finally assign the indices to the corresponding clusters for the new loop
		std::transform( iBegin, iEnd, indices.begin(), assign_indices< mean_type_iterator, BinaryOperator >( itMeanBegin, itMeanEnd, distanceFunc ) );
		
		if( diff_error < epsilon )
			break;
	}
	
	// and copy the indices to the corresponding output iterator
	std::copy( indices.begin(), indices.end(), indicesOut );
	
	// std::cout << "Exit after " << i << " iterations with " << diff_error <<  " mean of summarized differences.\n" ;
	return diff_error;
};

// /// overloaded function, see other k-means for detailed description
// template< typename InputIterator, typename OutputIterator >
// void k_means( const InputIterator iBeginValues, const InputIterator iEndValues, const std::size_t n_cluster, OutputIterator itIndices )
// {
	// typedef typename std::iterator_traits< InputIterator >::value_type VecType;
	// std::vector< VecType > means;
	// k_means( iBeginValues, iEndValues, n_cluster, std::back_inserter( means ), itIndices, *iBeginValues );
// };

/**
 * @ingroup math stochastic
 * @brief determines k centroids of clusters from a set of elements
 *
 * This function can be applied to nearly any container structure
 * providing access to the single container elements via input iterators.
 * Although designed for stl-containers * (e.g. \c std::vector,
 * \c std::list or \c std::set, etc ) the algorithm is not limited
 * to these ones but can be used via any pointer to data-structures.
 *
 * Example use case:\n
 * std::vector< Vector3d > points3d; // <- class to pick some \c k elements \n
 * std::size_t k; // <- defines the number of clusters to be determined \n
 * std::vector< Vector3d > points3dOut; // <- will be filled with the cluster centroids, storage can be allocated with \c reserve() \n
 * std::vector< std::size_t > indices; // <- will sign to with cluster a point belongs corresponds to the elements from the input iterators, storage can be allocated with \c reserve() \n
 * k_means( points3d.begin(), points3d.end(), k, std::back_inserter( points3dOut ), std::back_inserter( indices ) );\n
 * 
 * @tparam InputIterator type of input iterator to container of input elements
 * @tparam OutputIterator1 type of the output iterator to container of the clusters
 * @tparam OutputIterator2 type of the output iterator to container of the indices
 * @param iBeginValues \c iterator pointing to first element in the input container/storage class of the input elements ( usually \c begin() )
 * @param iEndValues \c iterator pointing behind the last element in the input container/storage class of the input elements ( usually \c end() )
 * @param n_cluster a value that signs the amount of clusters and corresponding centroids that are expected within the input elements
 * @param iBeginMean output \c iterator pointing to first element in container/storage class for storing the determined centroids ( usually \c begin() or \c std::back_inserter(container) )
 * @param itIndices output \c iterator pointing to first element in container/storage class for storing the indices to the centroids of the corresponding input elements ( usually \c begin() or \c std::back_inserter(container) )
 */ 
template< typename InputIterator, typename OutputIterator1, typename OutputIterator2 >
void k_means( const InputIterator iBeginValues, const InputIterator iEndValues, const std::size_t n_cluster, OutputIterator1 itCentroids, OutputIterator2 itIndices )
{
	typedef typename std::iterator_traits< InputIterator >::value_type vector_type;	
	typedef typename std::vector< vector_type > mean_container_type;
	typedef typename mean_container_type::iterator mean_type_iterator;
	
	// the sum of the different means, saved separately for each dimension.
	mean_container_type means;
	means.reserve( n_cluster );
	
	// copy_greedy( iBeginValues, iEndValues, n_cluster, std::back_inserter( means ) );
	// std::cout << "Means greedy " << means << std::endl;
	// copy_probability( iBeginValues, iEndValues, n_cluster, means.begin() ) );
	// std::cout << "Means probability " << means << std::endl;
	
	copy_probability( iBeginValues, iEndValues, n_cluster, std::back_inserter( means ), Distance< vector_type >() );
	
	k_means( iBeginValues, iEndValues, means.begin(), means.end(), itIndices, SquaredDistance< vector_type >() );
	
	// copy the resulting mean values to output iterator
	std::copy( means.begin(), means.end(), itCentroids );
};

} } } // namespace Ubitrack::Math::Stochastic

#endif //__UBITRACK_MATH_STOCHASTIC_K_MEANS_H__
