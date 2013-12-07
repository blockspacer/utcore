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
 * @ingroup math
 * @file
 * 6D pose, composed from vector and quaternion.
 * @author Florian Echtler <echtler@in.tum.de>
 */


#ifndef __POSE_H_INCLUDED__
#define __POSE_H_INCLUDED__

#include <utCore.h>
#include "cast_assign.h"
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include "Vector.h"
#include "Quaternion.h"


namespace Ubitrack { namespace Math {

//forward declaration to matrix template
template< typename T, std::size_t M, std::size_t N > class Matrix;


/**
 * @ingroup math
 * Represents a 6D pose, composed from a quaternion and a 3-dimensional translation vector.
 */
class UBITRACK_EXPORT Pose
{

	friend UBITRACK_EXPORT std::ostream& operator<<( std::ostream& s, const Pose& p );
	friend class ::boost::serialization::access;

	public:		

		/** doesn't make much sense, but sometimes we need a default constructor */
		Pose()
			: m_rotation( )
			, m_translation ( Vector < double, 3 > ( 0, 0, 0 ) )
		{}

		/**
		 * construct a pose from quaternion and translation
		 * @param q a rotation
		 * @param t a translation
		 */
		Pose( const Quaternion& q, const Vector< double, 3 >& t )
			: m_rotation( q )
			, m_translation( t )
		{}
		
		/**
		 * construct a pose from a 4x4 matrix
		 * @param m a 4x4 matrix
		 */
		Pose( const Math::Matrix< double, 0, 0 >& mat );
		
		/**
		 * scale the pose
		 * @return scaled pose
		 */
		 void scalePose( double scalingFactor )
		 {
			m_translation( 0 ) *= scalingFactor;
			m_translation( 1 ) *= scalingFactor;
			m_translation( 2 ) *= scalingFactor;			
			
			return;
		 }

		/**
		 * get the quaternion part
		 * @return rotation part of the pose
		 */
		const Quaternion& rotation() const
		{ return m_rotation; }

		/**
		 * get the translation part
		 * @return translation part of the pose
		 */
		const Vector< double, 3 >& translation() const
		{ return m_translation; }

		/**
		 * Store the pose in a Boost uBlas vector.
		 * The order is tx, ty, tz, qx, qy, qz, qw.
		 */
		template< class VType >
		void toVector( VType& v ) const
		{ 
			typedef typename VType::value_type T;
			boost::numeric::ublas::vector_range< VType > rotSub( v, boost::numeric::ublas::range( 3, 7 ) );
			m_rotation.toVector( rotSub );
			boost::numeric::ublas::vector_range< VType > posSub( v, boost::numeric::ublas::range( 0, 3 ) );
			vector_cast_assign( posSub, m_translation );
		}
		
		/**
		 * Retrieve a quaternion from a Boost uBlas vector.
		 * The order is tx, ty, tz, qx, qy, qz, qw.
		 */
		template< class VType >
		static Pose fromVector( const VType& v )
		{ return Pose( Quaternion::fromVector( boost::numeric::ublas::subrange( v, 3, 7 ) ), Vector< double, 3 >( v( 0 ), v( 1 ), v( 2 ) ) ); }
		
		// assignment and copy constructors are automatically generated by the compiler

		/**
		 * transform a vector using a pose
		 * @param x a vector
		 * @return the transformed vector
		 */
		Vector< double, 3 > operator*( const Vector< double, 3 >& x ) const;

		/**
		 * Multiplies two poses.
		 * If P (this) represents a transformation x_A = r_P x_B r_P^* + t_P from a coordinate system B to A
		 * and Q a transformation from C to B, then the function computes the transformation from C to B.
		 * @param Q a pose
		 * @return the transformed pose
		 */
		Pose operator*( const Pose& Q ) const;

		/**
		 * inverts a pose
		 * @return the inverted pose
		 */
		Pose operator~( ) const;

	protected:

		/// serialize Pose object
		template< class Archive > void serialize( Archive& ar, const unsigned int version )
		{
			ar & m_rotation;
			ar & m_translation;
		}

		Quaternion m_rotation;
		Vector< double, 3 >	m_translation;

};


/// stream output operator
UBITRACK_EXPORT std::ostream& operator<<( std::ostream& s, const Pose& p );


/**
 * performs a linear interpolation between two poses
 * using SLERP and vector interpolation
 * @param x first pose
 * @param y second pose
 * @param t interpolation point between 0.0 and 1.0
 * @return the new pose
 */
UBITRACK_EXPORT Pose linearInterpolate ( const Pose& x, const Pose& y, double t );

} } // namespace Ubitrack::Math

#endif // __POSE_H_INCLUDED__

