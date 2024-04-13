#include "greencoordinates3d.h"

GreenCoordinates3D::GreenCoordinates3D()
{

}

void GreenCoordinates3D::constructGreenCoordinates(const Vector3f& vertexPos, HalfEdgeMesh& cage) {
    //--- Initialize all coords as 0
    phiCoords.resize(cage.vertices.size());
    std::fill(phiCoords.begin(), phiCoords.end(), 0.0f);
    psiCoords.resize(cage.faces.size());
    std::fill(psiCoords.begin(), psiCoords.end(), 0.0f);

    //--- Calculate coords by iterating all faces
    int j = 0;
    for (Face& face : cage.faces) {
        for (HalfEdge* halfEdge : face.halfEdges) {
            halfEdge->vertex->calculatePosition = halfEdge->vertex->position - vertexPos;
        }

        Vector3f faceNormal = face.calculateNormal();
        Vector3f p = (face.halfEdges[0]->vertex->calculatePosition.dot(faceNormal)) * faceNormal;

        for (HalfEdge* halfEdge : face.halfEdges) {
            Vertex* thisVertex = halfEdge->vertex;
            Vertex* nextVertex = halfEdge->next->vertex;
            thisVertex->s = copysign(1.0, (thisVertex->calculatePosition - p).cross(nextVertex->calculatePosition - p).dot(faceNormal));
            thisVertex->I = gcTriInt(p, thisVertex->calculatePosition, nextVertex->calculatePosition, Vector3f(0,0,0));
            thisVertex->II = gcTriInt(Vector3f(0,0,0), nextVertex->calculatePosition, thisVertex->calculatePosition, Vector3f(0,0,0));
            thisVertex->q = nextVertex->calculatePosition.cross(thisVertex->calculatePosition);
            thisVertex->N = thisVertex->q / thisVertex->q.norm();
        }

        float ISum = 0;
        for (HalfEdge* halfEdge : face.halfEdges) {
            ISum += halfEdge->vertex->s * halfEdge->vertex->I;
        }
        float I = -abs(ISum);
        psiCoords.at(j) = -I;

        Vector3f omega = faceNormal * I;
        for (HalfEdge* halfEdge : face.halfEdges) {
            omega += halfEdge->vertex->N * halfEdge->vertex->II;
        }
        float epsilon = 0.00001; // TODO: Check how to set this
        if (omega.norm() > epsilon) {
            for (HalfEdge* halfEdge : face.halfEdges) {
                int vertexIdx = halfEdge->vertex->vertexIdx;
                phiCoords.at(vertexIdx) += (halfEdge->next->vertex->N.dot(omega)) / (halfEdge->next->vertex->N.dot(halfEdge->vertex->calculatePosition));
            }
        }

        j++;
    }
}

void GreenCoordinates3D::constructGreenCoordinatesExterior(const Vector3f& vertexPos, HalfEdgeMesh& cage) {
    //--- Construct the coordinates the same as internal at first
    constructGreenCoordinates(vertexPos, cage);

    //--- Add alphas and betas
    int j = 0;
    for (Face& face : cage.faces) {
        // TODO: Need to figure out a way to define the EXIT FACE !!!!!!
        if (face.calculateNormal() != Vector3f(0,-1,0)) {
            j++;
            continue;
        }

        vector<Vector3f> vList(3);
        std::fill(vList.begin(), vList.end(), Vector3f(0,0,0));

        for (int i = 0; i < 3; i++) {
            vList.at(i) = face.halfEdges[i]->vertex->position;
        }

        Vector3f faceNormal = face.calculateNormal();

        MatrixXf A(4, 4);
//        cout << vList.at(0) << endl;
        A << vList.at(0), vList.at(1), vList.at(2), faceNormal,
            1, 1, 1, 0;
//        cout << A << endl;
        Vector4f b;
        b << vertexPos, 1;
//        cout << b << endl << endl;

        Vector4f solution = A.colPivHouseholderQr().solve(b);
//        cout << solution << endl;

        for (int i = 0; i < 3; i++) {
            int vertexIdx = face.halfEdges[i]->vertex->vertexIdx;
            phiCoords.at(vertexIdx) += solution[i];
//            cout << phiCoords.at(vertexIdx) << endl;
        }

        psiCoords.at(j) += solution[3];
//        cout << psiCoords.at(j) << endl << endl;

        j++;
        break;
    }
}

float GreenCoordinates3D::gcTriInt(Vector3f p, Vector3f v1, Vector3f v2, Vector3f eta) {
    //--- Calculate alpha
    float alphaNominator = (v2 - v1).dot(p - v1);
    float alphaDenominator = (v2 - v1).norm() * (p - v1).norm();
    float alpha = acos(alphaNominator / alphaDenominator);

    //--- Calculate beta
    float betaNominator = (v1 - p).dot(v2 - p);
    float betaDenominator = (v1 - p).norm() * (v2 - p).norm();
    float beta = acos(betaNominator / betaDenominator);

    //--- Calculate lambda
    float lambda = (p - v1).norm() * (p - v1).norm() * sin(alpha) * sin(alpha);

    //--- Calculate c
    float c = (p - eta).norm() * (p - eta).norm();

    std::vector<float> thetaList(2, 0.0f);
    thetaList.at(0) = M_PI - alpha;
    thetaList.at(1) = M_PI - alpha - beta;
    std::vector<float> IList(2, 0.0f);
    for (int i = 0; i < 2; i++) {
        float theta = thetaList.at(i);
        float S = sin(theta);
        float C = cos(theta);

        float term1 = -copysign(1.0, S) * 0.5;
        float term2 = 2 * sqrt(c) * atan((sqrt(c) * C) / sqrt(lambda + S*S*c));
        float term3a = (2 * sqrt(lambda) * S * S) / ((1 - C) * (1 - C));
        float term3b = 1 - 2*c*C / (c * (1+C) + lambda + sqrt(lambda*lambda + lambda*c*S*S));
        float term3 = sqrt(lambda) * log(term3a * term3b);

        IList.at(i) = term1 * (term2 + term3);
    }

    float result = -1 / (4*M_PI) * abs(IList.at(0) - IList.at(1) - sqrt(c) * beta);

    return result;
}
