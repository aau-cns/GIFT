/*
    This file is part of GIFT.

    GIFT is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    GIFT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with GIFT.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Calibration.h"
#include "Eigen/SVD"

using namespace GIFT;

Eigen::Matrix3T initialisePinholeIntrinsics(const std::vector<cv::Mat>& homographies) {
    // Closed form solution to pinhole camera intrinsics
    // https://ieeexplore.ieee.org/stamp/stamp.jsp?arnumber=888718

    std::vector<Eigen::Matrix<ftype, 2, 6>> constraintMatrices(homographies.size());

    std::transform(homographies.begin(), homographies.end(), constraintMatrices.begin(), [](const cv::Mat& H) {
        Eigen::Matrix<ftype, 6, 1> v00, v01, v11;
        v00 << H.at<ftype>(0, 0) * H.at<ftype>(0, 0),
            H.at<ftype>(0, 0) * H.at<ftype>(1, 0) + H.at<ftype>(1, 0) * H.at<ftype>(0, 0),
            H.at<ftype>(1, 0) * H.at<ftype>(1, 0),
            H.at<ftype>(2, 0) * H.at<ftype>(0, 0) + H.at<ftype>(0, 0) * H.at<ftype>(2, 0),
            H.at<ftype>(2, 0) * H.at<ftype>(1, 0) + H.at<ftype>(1, 0) * H.at<ftype>(2, 0),
            H.at<ftype>(2, 0) * H.at<ftype>(2, 0);
        v01 << H.at<ftype>(0, 0) * H.at<ftype>(0, 1),
            H.at<ftype>(0, 0) * H.at<ftype>(1, 1) + H.at<ftype>(1, 0) * H.at<ftype>(0, 1),
            H.at<ftype>(1, 0) * H.at<ftype>(1, 1),
            H.at<ftype>(2, 0) * H.at<ftype>(0, 1) + H.at<ftype>(0, 0) * H.at<ftype>(2, 1),
            H.at<ftype>(2, 0) * H.at<ftype>(1, 1) + H.at<ftype>(1, 0) * H.at<ftype>(2, 1),
            H.at<ftype>(2, 0) * H.at<ftype>(2, 1);
        v11 << H.at<ftype>(0, 1) * H.at<ftype>(0, 1),
            H.at<ftype>(0, 1) * H.at<ftype>(1, 1) + H.at<ftype>(1, 1) * H.at<ftype>(0, 1),
            H.at<ftype>(1, 1) * H.at<ftype>(1, 1),
            H.at<ftype>(2, 1) * H.at<ftype>(0, 1) + H.at<ftype>(0, 1) * H.at<ftype>(2, 1),
            H.at<ftype>(2, 1) * H.at<ftype>(1, 1) + H.at<ftype>(1, 1) * H.at<ftype>(2, 1),
            H.at<ftype>(2, 1) * H.at<ftype>(2, 1);
        Eigen::Matrix<ftype, 2, 6> V;
        V << v01.transpose(), (v00 - v11).transpose();
        return V;
    });

    Eigen::Matrix<ftype, Eigen::Dynamic, 6> constraintMatrix(2 * constraintMatrices.size(), 6);
    for (int i = 0; i < constraintMatrices.size(); ++i) {
        constraintMatrix.block<2, 6>(2 * i, 0) = constraintMatrices[i];
    }

    // Compute the smallest right singular vector of the constraint matrix

    Eigen::BDCSVD<Eigen::MatrixXT> svd(constraintMatrix, Eigen::ComputeThinU | Eigen::ComputeThinV);
    Eigen::Matrix<ftype, 6, 1> bVector = svd.matrixV().block<6, 1>(0, 5);

    // Extract parameters
    const ftype &B11 = bVector(0), B12 = bVector(1), B22 = bVector(2), B13 = bVector(3), B23 = bVector(4),
                B33 = bVector(5);

    const ftype v0 = (B12 * B13 - B11 * B23) / (B11 * B22 - B12 * B12);
    const ftype lambda = B33 - (B13 * B13 + v0 * (B12 * B13 - B11 * B23)) / B11;
    const ftype alpha = sqrt(lambda / B11);
    const ftype beta = sqrt(lambda * B11 / (B11 * B22 - B12 * B12));
    const ftype gamma = -B12 * alpha * alpha * beta / lambda;
    const ftype u0 = gamma * v0 / alpha - B12 * alpha * alpha / lambda;

    Eigen::Matrix3T cameraMatrix = Eigen::Matrix3T::Identity();
    cameraMatrix(0, 0) = alpha;
    cameraMatrix(1, 1) = beta;
    cameraMatrix(0, 1) = gamma;
    cameraMatrix(0, 2) = u0;
    cameraMatrix(1, 2) = v0;

    return cameraMatrix;
}

GIFT::PinholeCamera initialisePoses(const std::vector<cv::Mat>& homographies, const Eigen::Matrix3T& cameraMatrix) {}