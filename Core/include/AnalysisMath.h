/*! Common math functions and definitions suitable for analysis purposes.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include <iostream>
#include <cmath>
#include <Math/PtEtaPhiE4D.h>
#include <Math/PtEtaPhiM4D.h>
#include <Math/PxPyPzE4D.h>
#include <Math/LorentzVector.h>
#include <Math/SMatrix.h>
#include <Math/VectorUtil.h>
#include <TVector3.h>
#include <TH1.h>
#include <TMatrixD.h>
#include <TLorentzVector.h>


#include "PhysicalValue.h"

namespace analysis {

using LorentzVectorXYZ = ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double>>;
using LorentzVectorM = ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<double>>;
using LorentzVectorE = ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiE4D<double>>;
using LorentzVector = LorentzVectorE;

using LorentzVectorXYZ_Float = ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<float>>;
using LorentzVectorM_Float = ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiM4D<float>>;
using LorentzVectorE_Float = ROOT::Math::LorentzVector<ROOT::Math::PtEtaPhiE4D<float>>;

template<unsigned n>
using SquareMatrix = ROOT::Math::SMatrix<double, n, n, ROOT::Math::MatRepStd<double, n>>;

template<unsigned n>
TMatrixD ConvertMatrix(const SquareMatrix<n>& m)
{
    TMatrixD result(n, n);
    for(unsigned k = 0; k < n; ++k) {
        for(unsigned l = 0; l < n; ++l)
            result[k][l] = m[k][l];
    }
    return result;
}

template<typename LVector>
TLorentzVector ConvertVector(const LVector& v)
{
    return TLorentzVector(v.Px(), v.Py(), v.Pz(), v.E());
}

//see AN-13-178
template<typename LVector1, typename LVector2>
double Calculate_MT(const LVector1& lepton_p4, const LVector2& met_p4)
{
    const double delta_phi = TVector2::Phi_mpi_pi(lepton_p4.Phi() - met_p4.Phi());
    return std::sqrt( 2.0 * lepton_p4.Pt() * met_p4.Pt() * ( 1.0 - std::cos(delta_phi) ) );
}

template<typename LVector1, typename LVector2, typename LVector3>
double Calculate_TotalMT(const LVector1& lepton1_p4, const LVector2& lepton2_p4, const LVector3& met_p4)
{
    const double mt_1 = Calculate_MT(lepton1_p4, met_p4);
    const double mt_2 = Calculate_MT(lepton2_p4, met_p4);
    const double mt_ll = Calculate_MT(lepton1_p4, lepton2_p4);
    return std::sqrt(std::pow(mt_1, 2) + std::pow(mt_2, 2) + std::pow(mt_ll, 2));
}

// from DataFormats/TrackReco/interface/TrackBase.h
template<typename Point>
double Calculate_dxy(const Point& legV, const Point& PV, const LorentzVector& leg)
{
    return ( - (legV.x()-PV.x()) * leg.Py() + (legV.y()-PV.y()) * leg.Px() ) / leg.Pt();
}

// from DataFormats/TrackReco/interface/TrackBase.h
inline double Calculate_dz(const TVector3& trkV, const TVector3& PV, const TVector3& trkP)
{
  return (trkV.z() - PV.z()) - ( (trkV.x() - PV.x()) * trkP.x() + (trkV.y() - PV.y()) * trkP.y() ) / trkP.Pt()
                               * trkP.z() / trkP.Pt();
}

template<typename LVector1, typename LVector2, typename LVector3>
double Calculate_Pzeta(const LVector1& l1_p4, const LVector2& l2_p4, const LVector3& met_p4)
{
    const auto ll_p4 = l1_p4 + l2_p4;
    const TVector2 ll_p2(ll_p4.Px(), ll_p4.Py());
    const TVector2 met_p2(met_p4.Px(), met_p4.Py());
    const TVector2 ll_s = ll_p2 + met_p2;
    const TVector2 l1_u(std::cos(l1_p4.Phi()), std::sin(l1_p4.Phi()));
    const TVector2 l2_u(std::cos(l2_p4.Phi()), std::sin(l2_p4.Phi()));
    const TVector2 ll_u = l1_u + l2_u;
    const double ll_u_met = ll_s * ll_u;
    const double ll_mod = ll_u.Mod();
    return ll_u_met / ll_mod;
}

template<typename LVector1, typename LVector2>
double Calculate_visiblePzeta(const LVector1& l1_p4, const LVector2& l2_p4)
{
    const auto ll_p4 = l1_p4 + l2_p4;
    const TVector2 ll_p2(ll_p4.Px(), ll_p4.Py());
    const TVector2 l1_u(std::cos(l1_p4.Phi()), std::sin(l1_p4.Phi()));
    const TVector2 l2_u(std::cos(l2_p4.Phi()), std::sin(l2_p4.Phi()));
    const TVector2 ll_u = l1_u + l2_u;
    const double ll_p2u = ll_p2 * ll_u;
    const double ll_mod = ll_u.Mod();
    return ll_p2u / ll_mod;
}

template<typename LVector1, typename LVector2, typename LVector3, typename LVector4, typename LVector5 >
std::pair<double, double> Calculate_mass_top(const LVector1& lepton1_p4, const LVector2& lepton2_p4, const LVector3& bjet_1, const LVector4& bjet_2, const LVector5& met_p4){
    const double mass_top = 173.21;
    const auto a1 = lepton1_p4 + bjet_1 + met_p4;
    const auto b1 = lepton2_p4 + bjet_2;
    auto a2 = lepton1_p4 + bjet_1;
    auto b2 = lepton2_p4 + bjet_2 + met_p4;
    auto a3 = lepton1_p4 + bjet_2 + met_p4;
    auto b3 = lepton2_p4 + bjet_1;
    auto a4 = lepton1_p4 + bjet_2;
    auto b4 = lepton2_p4 + bjet_1 + met_p4;

    double d1 = pow(std::abs(a1.mass() - mass_top),2) + pow (std::abs(b1.mass() - mass_top),2);
    double d2 = pow(std::abs(a2.mass() - mass_top),2) + pow (std::abs(b2.mass() - mass_top),2);
    double d3 = pow(std::abs(a3.mass() - mass_top),2) + pow (std::abs(b3.mass() - mass_top),2);
    double d4 = pow(std::abs(a4.mass() - mass_top),2) + pow (std::abs(b4.mass() - mass_top),2);
    std::pair<double, double> pair_mass_top;
    if (d1<d2 && d1<d3 && d1<d4) {
        pair_mass_top.first = a1.mass();
        pair_mass_top.second = b1.mass();
    }
    if (d2<d1 && d2<d3 && d2<d4) {
        pair_mass_top.first = a2.mass();
        pair_mass_top.second = b2.mass();
    }
    if (d3<d1 && d3<d2 && d3<d4) {
        pair_mass_top.first = a3.mass();
        pair_mass_top.second = b3.mass();
    }
    if (d4<d1 && d4<d3 && d4<d2) {
        pair_mass_top.first = a4.mass();
        pair_mass_top.second = b4.mass();
    }
    return pair_mass_top;
}

template<typename LVector1, typename LVector2, typename LVector3 >
double Calculate_dR_boosted(const LVector1& particle_1, const LVector2& particle_2, const LVector3& h){
    const analysis::LorentzVectorXYZ h_vector(h.px(),h.py(),h.pz(),h.e());
    const auto boosted_1 = ROOT::Math::VectorUtil::boost(particle_1, h_vector.BoostToCM());
    const auto boosted_2 = ROOT::Math::VectorUtil::boost(particle_2, h_vector.BoostToCM());
    return ROOT::Math::VectorUtil::DeltaR(boosted_1, boosted_2); // R between the two final state particle in the h rest frame
}


template<typename LVector1, typename LVector2, typename LVector3, typename LVector4, typename LVector5,  typename LVector6>
double Calculate_phi_4bodies(const LVector1& lepton1, const LVector2& lepton2, const LVector3& bjet1, const LVector4& bjet2, const LVector5& svfit, const LVector6& bb){
    const auto H = bb + svfit;
    const analysis::LorentzVectorXYZ vec_H(H.px(), H.py(),H.pz(),H.e());
    const auto boosted_l1 = ROOT::Math::VectorUtil::boost(lepton1, vec_H.BoostToCM());
    const auto boosted_l2 = ROOT::Math::VectorUtil::boost(lepton2, vec_H.BoostToCM());
    const auto boosted_j1 = ROOT::Math::VectorUtil::boost(bjet1, vec_H.BoostToCM());
    const auto boosted_j2 = ROOT::Math::VectorUtil::boost(bjet2, vec_H.BoostToCM());
    const TVector3 vec_l1(boosted_l1.px(),boosted_l1.py(),boosted_l1.pz());
    const TVector3 vec_l2(boosted_l2.px(),boosted_l2.py(),boosted_l2.pz());
    const TVector3 vec_j1(boosted_j1.px(),boosted_j1.py(),boosted_j1.pz());
    const TVector3 vec_j2(boosted_j2.px(),boosted_j2.py(),boosted_j2.pz());
    const auto n1 = vec_l1.Cross(vec_l2);
    const auto n2 = vec_j1.Cross(vec_j2);
    return ROOT::Math::VectorUtil::Angle(n1, n2); //angle between the decay planes of the four final state elements expressed in the H rest frame
}

template<typename LVector1, typename LVector2>
double Calculate_theta_star(const LVector1& svfit, const LVector2& bb){
    const auto H = bb + svfit;
    const analysis::LorentzVectorXYZ vec_H(H.px(),H.py(),H.pz(),H.e());
    const auto boosted_h = ROOT::Math::VectorUtil::boost(svfit, vec_H.BoostToCM());
    return std::acos(ROOT::Math::VectorUtil::CosTheta(boosted_h, ROOT::Math::Cartesian3D<>(0, 0, 1))); // Is the production angle of the h_tautau defined in the H rest frame
}

template<typename LVector1, typename LVector2, typename LVector3, typename LVector4>
double Calculate_phi_star(const LVector1& object1, const LVector2& object2, const LVector3& svfit, const LVector4& bb){
    const auto H = bb + svfit;
    const analysis::LorentzVectorXYZ vec_H(H.px(),H.py(),H.pz(),H.e());
    const auto boosted_1 = ROOT::Math::VectorUtil::boost(object1, vec_H.BoostToCM());
    const auto boosted_2 = ROOT::Math::VectorUtil::boost(object2, vec_H.BoostToCM());
    const auto boosted_h = ROOT::Math::VectorUtil::boost(svfit, vec_H.BoostToCM());
    const TVector3 vec_h(boosted_h.px(),boosted_h.py(),boosted_h.pz());
    const TVector3 vec_1(boosted_1.px(),boosted_1.py(),boosted_1.pz());
    const TVector3 vec_2(boosted_2.px(),boosted_2.py(),boosted_2.pz());
    TVector3 z_axis(0,0,1);
    const auto n1 = vec_1.Cross(vec_2);
    const auto n3 = vec_h.Cross(z_axis);
    return ROOT::Math::VectorUtil::Angle(n1,n3);
}



inline PhysicalValue Integral(const TH1D& histogram, bool include_overflows = true)
{
    using limit_pair = std::pair<Int_t, Int_t>;
    const limit_pair limits = include_overflows ? limit_pair(0, histogram.GetNbinsX() + 1)
                                                : limit_pair(1, histogram.GetNbinsX());

    double error = 0;
    const double integral = histogram.IntegralAndError(limits.first, limits.second, error);
    return PhysicalValue(integral, error);
}

inline void RenormalizeHistogram(TH1D& histogram, const PhysicalValue& norm, bool include_overflows = true)
{
    histogram.Scale(norm.GetValue() / Integral(histogram,include_overflows).GetValue());
}

} // namespace analysis
