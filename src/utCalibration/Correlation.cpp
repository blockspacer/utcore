
/**
 * @ingroup tracking_algorithms
 * @file
 * Implementation of Correlation
 *
 * @author Manuel Huber <huberma@in.tum.de>
 */

#include "Correlation.h"
#ifdef HAVE_LAPACK

#include <utMath/Matrix.h>
#include <utUtil/Exception.h>

namespace Ubitrack { namespace Calibration {

double computeCorrelationDirect ( const std::vector< double >& left,
								  const std::vector< double >& right)
{
	double res = 0.0;
	size_t len = left.size() < right.size() ? left.size() : right.size();

	double var1 = 0.0;
	double var2 = 0.0;

	for (int i=0; i<len; i++) {
		res += left[i]*right[i];
		var1 += left[i]*left[i];
		var2 += right[i]*right[i];
	}

	double denom = sqrt(var1*var2);

	return res/denom;
}

double computeCorrelation ( const std::vector< double >& left,
							const std::vector< double >& right)
{
	if (left.size() == 0 && right.size() == 0) {
		return 1.0f;
	}

	return computeCorrelationDirect(left,right);
}

} } // namespace Ubitrack::Calibration

#endif // HAVE_LAPACK
