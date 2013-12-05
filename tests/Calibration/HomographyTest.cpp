
#include <utMath/Vector.h>
#include <utMath/Matrix.h>
#include <utMath/Random/Vector.h>
#include <utMath/Random/Rotation.h>
#include <utCalibration/Homography.h>
#include <utMath/Functors/MatrixFunctors.h>
#include <utMath/Functors/VectorFunctors.h>
#include <utCalibration/2D3DPoseEstimation.h> // for PoseFromHomography

#include "../tools.h"

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>

using namespace Ubitrack::Math;
namespace ublas = boost::numeric::ublas;

template< typename T >
void TestHomographyDLTIdentity( const T epsilon )
{
	// check if standard corners return identity
	std::vector< Vector< 2, T > > stdCorners( 4 );
	for ( std::size_t i = 0; i < 4; i++ )
	{
		stdCorners[ i ][ 0 ] = ( i & 2 )         ? 0.5 : -0.5;
		stdCorners[ i ][ 1 ] = ( ( i + 1 ) & 2 ) ? -0.5 : 0.5;
	}
	
	Matrix< 3, 3, T > H( Ubitrack::Calibration::homographyDLT( stdCorners, stdCorners ) );
	BOOST_CHECK_SMALL( homMatrixDiff( H, Matrix< 3, 3, T >::identity() ), epsilon );
}

template< typename T >
void TestSquareHomography( const std::size_t n_runs, const T epsilon )
{
	// check if standard corners return identity
	std::vector< Vector< 2, T > > stdCorners( 4 );
	for ( std::size_t i = 0; i < 4; i++ )
	{
		stdCorners[ i ][ 0 ] = ( i & 2 )         ? 0.5f : -0.5f;
		stdCorners[ i ][ 1 ] = ( ( i + 1 ) & 2 ) ? -0.5f : 0.5f;
	}
	
	Matrix< 3, 3, T > H( Ubitrack::Calibration::squareHomography( stdCorners ) );
	BOOST_CHECK_SMALL( homMatrixDiff( H, Matrix< 3, 3, T >::identity() ), epsilon );
	
	
	for ( std::size_t iTest = 0; iTest < n_runs; iTest++ )
	{
		Matrix< 3, 3, T > Htest;
		randomMatrix( Htest );
					
		// transform standard corners by Htest
		std::vector< Vector< 2, T > > tCorners( 4 );
		for ( std::size_t i = 0; i < 4; i++ )
		{
			Vector< 3, T > x( stdCorners[ i ] ( 0 ), stdCorners[ i ] ( 1 ), 1. );
			Vector< 3, T > xp = ublas::prod( Htest, x );
			tCorners[ i ] = ublas::subrange( xp, 0, 2 ) / xp( 2 );
		}
			
		H = Ubitrack::Calibration::squareHomography( tCorners );
		BOOST_CHECK_SMALL( homMatrixDiff( H, Htest ), epsilon );
	}
}
template < typename T >
void TestHomographyDLT( const std::size_t n_runs, const T epsilon )	
{
	// initialize random number generation.
	typename Random::Vector< 2, T >::Uniform randVector( -100, 100 ); // some 3D points
	
	// do some tests with random homographies
	for ( std::size_t iTest = 0; iTest < n_runs; iTest++ )
	{
		// create a random homography
		Matrix< 3, 3, T > Htest;
		randomMatrix( Htest );
				
		// create & transform random points
		// use at least 10 correspondences, as randomness may lead to poorly conditioned problems...
		const std::size_t n( Random::distribute_uniform< std::size_t >( 10, 50 ) );
		
		std::vector< Vector< 2, T > > fromPoints;
		fromPoints.reserve( n );
		std::generate_n ( std::back_inserter( fromPoints ), n,  randVector );
		
		std::vector< Vector< 2, T > > toPoints( n );
	
		for ( std::size_t i = 0; i < n; ++i )
		{
			Vector< 3, T > x( fromPoints[ i ]( 0 ), fromPoints[ i ]( 1 ), 1. );
			Vector< 3, T > xp = ublas::prod( Htest, x );
			toPoints[ i ] = ublas::subrange( xp, 0, 2 ) / xp( 2 );
		}
		
		Matrix< 3, 3, T > H = Ubitrack::Calibration::homographyDLT( fromPoints, toPoints );

		BOOST_CHECK_SMALL( homMatrixDiff( H, Htest ), epsilon );
	}
}


template< typename T >
void TestPoseFromHomography( const std::size_t n_runs, const T epsilon )
{
	typename Random::Quaternion< T >::Uniform randQuat;
	typename Random::Vector< 3, T >::Uniform randTranslation( -10, 10 ); //translation
	typename Random::Vector< 2, T >::Uniform randPositions( -100, 100 ); //position on the floor ( set z = 0).0 );
	// typename Random::Vector< 3, T >::Uniform randPositionNoise( -0.3, 0.3 ); // translation uniform noise
	const Vector< 2, T > screenResolution( 640, 480 );
	
	
	// run test for several times
	for( std::size_t i( 0 ); i< n_runs; ++i )
	{
			// random intrinsics matrix, never changes, assume always the same camera
		Matrix< 3, 3, T > cam( Matrix< 3, 3, T >::identity() );
		cam( 0, 0 ) = Random::distribute_uniform< T >( 500, 800 );
		cam( 1, 1 ) = Random::distribute_uniform< T >( 500, 800 );
		 //take care of ubitracks camera interpretation -> last column negative entries
		cam( 0, 2 ) = -screenResolution[ 0 ] * 0.5;
		cam( 1, 2 ) = -screenResolution[ 1 ] * 0.5;
		cam( 2, 2 ) = -1;
	
		// random pose
		Quaternion rot( randQuat( ) );
		Vector< 3, T > trans ( randTranslation() );
		trans( 2 ) = Random::distribute_uniform< T >( 1, 10 );
		Pose floor2Cam1 ( rot, trans );
	
		// projection to 2D image plane
		Matrix< 3, 4, T > projection( rot, trans );
		projection = boost::numeric::ublas::prod( cam, projection );
		
		// some (n) random points (homography => minimum 4 points)
		//const std::size_t n( 5 );
		const std::size_t n( Random::distribute_uniform< std::size_t >( 10, 30 ) );
		
		std::vector< Ubitrack::Math::Vector< 2, T > > ptsFloor;
		ptsFloor.reserve( n );
		std::generate_n ( std::back_inserter( ptsFloor ), n,  randPositions );
		
		std::vector< Ubitrack::Math::Vector< 2, T > > ptsCamera;
		ptsCamera.reserve( n );
		std::transform( ptsFloor.begin(), ptsFloor.end(), std::back_inserter( ptsCamera ), Functors::ProjectVector< T >( projection ) );
		
		
		Matrix< 3, 3, T > H = Ubitrack::Calibration::homographyDLT(  ptsFloor, ptsCamera );
		Matrix< 3, 3, T > invK( Functors::matrix_inverse()( cam ) );
		
		Pose floor2Cam2 = Ubitrack::Calibration::poseFromHomography( H, invK );

		Matrix< 3, 4, T > floor2CamMat1( floor2Cam1 );
		Matrix< 3, 4, T > floor2CamMat2( floor2Cam2 );
		BOOST_CHECK_SMALL( matrixDiff( floor2CamMat1, floor2CamMat2 ) , epsilon );
	}
}

void TestHomography()
{
	TestHomographyDLTIdentity< double >( 1e-6 );
	TestSquareHomography< double >( 1000, 1e-6 );
	TestHomographyDLT< double >( 1000, 1e-6 );
	TestPoseFromHomography< double >( 1000, 1e-6 );
	
	TestHomographyDLTIdentity< float >( 1e-3f );
	TestSquareHomography< float >( 1000, 1e-2f );
	TestHomographyDLT< float >( 1000, 1e-2f );
	TestPoseFromHomography< float >( 1000, 1e-2f );
}
