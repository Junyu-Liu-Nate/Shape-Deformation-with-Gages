#include "gchigherorder2d.h"

GCHigherOrder2D::GCHigherOrder2D()
{

}

void GCHigherOrder2D::constructGCHigherOrder(const Vector2f& vertexPos, vector<TwoDVertex> cagePoints, vector<TwoDEdge> cageEdges) {
    //--- Initialize all coords as 0-valued vectors
    phiCoords.resize(cagePoints.size());
    fill(phiCoords.begin(), phiCoords.end(), vector<float>(degree + 1, 0.0f));
    psiCoords.resize(cageEdges.size());
    fill(psiCoords.begin(), psiCoords.end(), vector<float>(degree + 1, 0.0f));

    //--- Calculate coords by iterating all edges
    for(int r = 0; r < cageEdges.size(); r++) {
        Vector2f v0 = cageEdges.at(r).edge.first->position;
        int phiIdx = cageEdges.at(r).edge.first->idx;
        Vector2f v1 = cageEdges.at(r).edge.second->position;

        gcHigherOrderEdge(vertexPos, v0, v1, phiCoords.at(phiIdx), psiCoords.at(r));
    }
}

void GCHigherOrder2D::gcHigherOrderEdge(const Vector2f& eta, const Vector2f& v0, const Vector2f& v1, vector<float>& phi, vector<float>& psi) {
    float x = ((eta - v0) / (eta - v0).norm()).dot((v1 - v0) / (v1 - v0).norm());

    float y = sqrt(1 - pow(x, 2));

    complex<float> complexNumber(x, y);
    float magnitudeNumerator = (eta - v0).norm();
    float magnitudeDenominator = (v1 - v0).norm();
    complex<float> omega = (magnitudeNumerator / magnitudeDenominator) * complexNumber;

    float B = 1 / (2 * M_PI * pow((v1 - v0).norm(), 2));

    float Dreal = 0.5 * log(1 + (1 - 2 * omega.real()) / norm(omega));
    float Dimag = atan2(omega.imag(), norm(omega) - omega.real());
    complex<float> W(Dreal, Dimag);

    float F;
    float epsilon = 1e-6;
    if (abs(omega.imag()) < epsilon) {
        F = B / (norm(omega) - omega.real());
    }
    else {
        F = (B * W.imag()) / omega.imag();
    }

    vector<float> F_list;
    F_list.resize(degree + 2); // The length of F_list is num_of_degree+2
    for (int i = 0; i < degree + 2; i++) {
        F_list.at(i) = F;
        F = omega.real() * F + B * W.real();
        W = omega * W + complex<float>(1.0 / (i + 1.0), 0);
    }

    Vector2f temp = v1 - v0;
    Vector2f tempTranspose = Vector2f(temp.y(), -temp.x());
    float alpha = - (eta - v0).dot(tempTranspose);
    float beta = - (eta - v0).dot((v1 - v0));
    float gamma = pow((v1 - v0).norm(), 2);
    float delta = (-1 / (2 * M_PI)) * log((v1 - eta).norm());

    for (int i = 0; i < degree + 1; i++) {
        phi.at(i) = alpha * F_list.at(i);
        psi.at(i) = beta * F_list.at(i) + gamma * F_list.at(i + 1) + delta;
    }

    psi.at(0) = 0;
}
