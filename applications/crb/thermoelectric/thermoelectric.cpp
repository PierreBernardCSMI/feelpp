//! -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t  -*-
//!
//! This file is part of the Feel++ library
//!
//! This library is free software; you can redistribute it and/or
//! modify it under the terms of the GNU Lesser General Public
//! License as published by the Free Software Foundation; either
//! version 2.1 of the License, or (at your option) any later version.
//!
//! This library is distributed in the hope that it will be useful,
//! but WITHOUT ANY WARRANTY; without even the implied warranty of
//! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//! Lesser General Public License for more details.
//!
//! You should have received a copy of the GNU Lesser General Public
//! License along with this library; if not, write to the Free Software
//! Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//!
//! @file
//! @author <you>
//! @date 15 Jun 2017
//! @copyright 2017 Feel++ Consortium
//!
//!

#include <feel/feelcrb/crbplugin.hpp>
#include "thermoelectric.hpp"

namespace Feel {

Thermoelectric::Thermoelectric()
    : super_type( "thermoelectric" )
{}

Thermoelectric::Thermoelectric( mesh_ptrtype mesh )
    : super_type( "thermoelectric" )
{
    this->M_mesh = mesh;
}

int Thermoelectric::Qa()
{
    auto bc = M_modelProps->boundaryConditions();
    auto materials = M_modelProps->Materials();
    return 2*materials.size() + bc["potential"]["Dirichlet"].size() +bc["potential"]["Neumann"].size()
           + bc["temperature"]["Robin"].size() + bc["temperature"]["Dirichlet"].size();
}

int Thermoelectric::mQA( int q )
{
    return 1;
}

int Thermoelectric::Nl()
{
    return 1;
}

int Thermoelectric::Ql( int l)
{
    auto bc = M_modelProps->boundaryConditions();
    switch(l) {
    case 0:
        return 1 + bc["potential"]["Dirichlet"].size() + bc["temperature"]["Robin"].size() + bc["temperature"]["Dirichlet"].size();
    default:
        return 0;
    }
}

int Thermoelectric::mLQF( int l, int q )
{
    switch( l )
    {
    case 0:
        return mCompliantQ(q);
    default:
        return 0;
    }
}

int Thermoelectric::mCompliantQ(int q )
{
    auto eimGradGrad = this->scalarDiscontinuousEim()[0];
    auto bc = M_modelProps->boundaryConditions();
    if( q == 0 )
        return eimGradGrad->mMax();
    else if( q < 1+bc["potential"]["Dirichlet"].size()+bc["potential"]["Neumann"].size() )
        return 1;
    else if( q < 1+bc["potential"]["Dirichlet"].size() + bc["potential"]["Neumann"].size()
                 +bc["temperature"]["Robin"].size() +bc["temperature"]["Dirichlet"].size()  )
        return 1;
    else
        return 0;
}

void Thermoelectric::resizeQm()
{
    M_Aqm.resize( Qa());
    M_betaAqm.resize( Qa() );
    for( int q = 0; q < Qa(); ++q )
    {
        M_Aqm[q].resize(mQA(q), backend()->newMatrix(Xh, Xh ) );
        M_betaAqm[q].resize(mQA(q));
    }

    M_Fqm.resize(Nl());
    M_betaFqm.resize(Nl());
    for( int l = 0; l < Nl(); ++l )
    {
        M_Fqm[l].resize(Ql(l));
        M_betaFqm[l].resize(Ql(l));
        for( int q = 0; q < Ql(l); ++q )
        {
            M_Fqm[l][q].resize(mLQF(l, q), backend()->newVector(Xh) );
            M_betaFqm[l][q].resize(mLQF(l, q) );
        }
    }

    M_InitialGuess.resize(1);
    M_InitialGuess[0].resize(1);
    M_InitialGuess[0][0] = Xh->elementPtr();
}

void Thermoelectric::initModel()
{
    Feel::cout << "initModel" << std::endl;

    M_modelProps = boost::make_shared<prop_type>(Environment::expand( soption("thermoelectric.filename")));

    auto parameters = M_modelProps->parameters();

    // Création de l'espace pour mu
    Dmu->setDimension(parameters.size());
    auto mu_min = Dmu->element();
    auto mu_max = Dmu->element();


    int i = 0;

    for( auto const& parameterPair : parameters )
    {
        mu_min(i) = parameterPair.second.min();
        mu_max(i) = parameterPair.second.max();
        Dmu->setParameterName(i++, parameterPair.first );
    }


    Dmu->setMin(mu_min);
    Dmu->setMax(mu_max);
    M_mu = Dmu->element();

    if( !M_mesh )
        M_mesh = loadMesh( new mesh_type );
    this->setFunctionSpaces(functionspace_type::New( M_mesh ) );

    if( !pT )
        pT = element_ptrtype( new element_type( Xh ) );

    M_V = pT->template elementPtr<0>();
    M_T = pT->template elementPtr<1>();

    auto JspaceEim = J_space_type::New( M_mesh );

    auto Pset = this->Dmu->sampling();

    int Ne = ioption(_name="thermoelectric.trainset-eim-size");
    std::vector<size_type> N(parameterSpace()->dimension(), Ne);

    std::string supersamplingname =(boost::format("DmuEim-Ne%1%-generated-by-master-proc") %Ne ).str();
    std::ifstream file ( supersamplingname );
    bool all_proc_same_sampling=true;

    if( ! file )
    {
        Pset->equidistributeProduct( N , all_proc_same_sampling , supersamplingname );
        Pset->writeOnFile( supersamplingname );
    }
    else
    {
        Pset->clear();
        Pset->readFromFile(supersamplingname);
    }

    auto eim_gradgrad = eim( _model=boost::dynamic_pointer_cast<Thermoelectric>(this->shared_from_this() ),
                             _element=*M_V,
                             _parameter=M_mu,
                             _expr=cst_ref(M_mu.parameterNamed("sigma"))*inner(gradv(*M_V)),
                             _space=JspaceEim,
                             _name="eim_gradgrad",
                             _sampling=Pset );
    this->addEimDiscontinuous( eim_gradgrad );

    this->resizeQm();
    this->decomposition();
}

void Thermoelectric::decomposition()
{
    auto UT = Xh->element();
    auto VP = Xh->element();
    auto u = UT.template element<0>();
    auto v = VP.template element<0>();
    auto t = UT.template element<1>();
    auto p = VP.template element<1>();

    auto gamma = doption("thermoelectric.gamma");

    auto eimGradGrad = this->scalarDiscontinuousEim()[0];
    auto bc = M_modelProps->boundaryConditions();
    auto materials = M_modelProps->materials();

    /************** Right hand side **************/
    // electro

    int idx = 0;
    for (auto const& mat : materials ){
        auto a0 = form2(_test=Xh, _trial=Xh);
        a0 = integrate( markedelements(M_mesh, mat.marker()),
                        inner(gradt(u),grad(v)) );
        M_Aqm[idx++][0] = a0.matrixPtr();

    }

    // thermo
    for (auto const& mat : materials){
        auto a1 = form2(_test=Xh, _trial=Xh);
        a1 = integrate( markedelements(M_mesh,mat.marker()),
                        inner(gradt(t), grad(p)) );
        M_Aqm[idx++][0] = a1.matrixPtr();
    }


    for( auto const& exAtM : bc["potential"]["Dirichlet"] )
    {
        auto aVD = form2(_test=Xh, _trial=Xh);
        aVD += integrate( markedfaces(M_mesh, exAtM.marker() ),
                          gamma/hFace()*inner(idt(u),id(v))
                          -inner(gradt(u)*N(),id(v))
                          -inner(grad(v)*N(),idt(u)) );
        M_Aqm[idx++][0] = aVD.matrixPtr();
    }

    for( auto const& exAtM : bc["potential"]["Neumann"] )
    {
        auto aVN = form2(_test=Xh, _trial=Xh);
        aVN += integrate( markedfaces(M_mesh, exAtM.marker() ),
                          id(v) );
        M_Aqm[idx++][0] = aVD.matrixPtr();
    }

    for( auto const& exAtM : bc["temperature"]["Dirichlet"] )
    {
        auto aTD = form2(_test=Xh, _trial=Xh);
        aTD += integrate( markedfaces(M_mesh, exAtM.marker() ),
                          gamma/hFace()*inner(idt(t),id(p))
                          -inner(gradt(t)*N(),id(p))
                          -inner(grad(p)*N(),idt(t)) );
        M_Aqm[idx++][0] = aTD.matrixPtr();
    }

    for( auto const& exAtM : bc["temperature"]["Robin"] )
    {
        auto aTR = form2(_test=Xh, _trial=Xh);
        aTR += integrate( markedfaces(M_mesh, exAtM.marker() ),
                          inner(id(t), idt(p)) );
        M_Aqm[idx++][0] = aTR.matrixPtr();
    }

    /************** Left hand side **************/

    idx = 0;
    for (auto const& mat : materials){
        for( int m = 0; m < eimGradGrad->mMax(); ++m )
        {
            auto f0 = form1(_test=Xh);
            f0 = integrate(markedelements(M_mesh, mat.marker()),
                           inner(id(p), idv(eimGradGrad->q(m))) );
            M_Fqm[0][idx++][m] = f0.vectorPtr();
        }

    }

    for( auto const& exAtM : bc["potential"]["Dirichlet"] )
    {
        auto fVD = form1(_test=Xh);
        fVD = integrate( markedfaces(M_mesh, exAtM.marker() ),
                         gamma/hFace()*id(v) -  grad(v)*N() );
        M_Fqm[0][idx++][0] = fVD.vectorPtr();
    }

    for( auto const& exAtM : bc["temperature"]["Dirichlet"] )
    {
        auto fTD = form1(_test=Xh);
        fTD = integrate( markedfaces(M_mesh, exAtM.marker() ),
                         gamma/hFace()*id(p) -  grad(p)*N() );
        M_Fqm[0][idx++][0] = fTD.vectorPtr();
    }

    for( auto const& exAtM : bc["temperature"]["Robin"] )
    {
        auto fTR = form1(_test=Xh);
        fTR = integrate( markedfaces(M_mesh, exAtM.marker() ),
                         idt(p) );
        M_Fqm[0][idx++][0] = fTR.vectorPtr();
    }


    // Energy matrix
    auto m = form2(_test=Xh, _trial=Xh);
    m = integrate( elements(M_mesh),
                   inner(gradt(u),grad(v)) + inner(grad(t), gradt(p)) );
    M_energy_matrix = m.matrixPtr();
}

Thermoelectric::beta_vector_type
Thermoelectric::computeBetaInitialGuess( parameter_type const& mu )
{
    M_betaInitialGuess.resize( 1 );
    M_betaInitialGuess[0].resize( 1 );
    M_betaInitialGuess[0][0] = 1;
    return this->M_betaInitialGuess;
}

Thermoelectric::beta_type
Thermoelectric::computeBetaQm( element_type const& T, parameter_type const& mu )
{
    auto eimGradGrad = this->scalarDiscontinuousEim()[0];
    auto betaEimGradGrad = eimGradGrad->beta( mu, T );
    this->fillBetaQm(mu, betaEimGradGrad);
    return boost::make_tuple( this->M_betaAqm, this->M_betaFqm);
}

Thermoelectric::beta_type
Thermoelectric::computeBetaQm( vectorN_type const& urb, parameter_type const& mu )
{
    auto eimGradGrad = this->scalarDiscontinuousEim()[0];
    auto betaEimGradGrad = eimGradGrad->beta( mu, urb );
    this->fillBetaQm(mu, betaEimGradGrad);
    return boost::make_tuple( this->M_betaAqm, this->M_betaFqm);
}

Thermoelectric::beta_type
Thermoelectric::computeBetaQm( parameter_type const& mu )
{
    auto eimGradGrad = this->scalarDiscontinuousEim()[0];
    auto betaEimGradGrad = eimGradGrad->beta( mu );
    this->fillBetaQm(mu, betaEimGradGrad);
    return boost::make_tuple( this->M_betaAqm, this->M_betaFqm);
}

void Thermoelectric::fillBetaQm( parameter_type const& mu, vectorN_type betaEimGradGrad )
{
    auto eimGradGrad = this->scalarDiscontinuousEim()[0];
    auto bc = M_modelProps->boundaryConditions();
    auto materials = M_modelProps->materials();
    // auto betaEimGradGrad = eimGradGrad->beta( mu );

    int idx = 0;

    for (auto const& mat : materials){
        M_betaAqm[idx++][0] = mu.parameterNamed(mat.getString("sigma"));
    }

    for (auto const& mat : materials){
        M_betaAqm[idx++][0] = mu.parameterNamed(mat.getString("k"));
    }

    //! A vérifier
    for( auto const& exAtM : bc["potential"]["Dirichlet"] )
        M_betaAqm[idx++][0] = mu.parameterNamed(materials[exAtM.material()].getString("sigma"));


    //! Devant mon expression Neumann j'ai -I/S_gammaIn, est-ce qu'il faut que je rajoute S_gammaIn ici?
    for( auto const& exAtM : bc["potential"]["Neumann"] )
        M_betaAqm[idx++][0] = mu.parameterNamed("I");


    for( auto const& exAtM : bc["temperature"]["Dirichlet"] )
        M_betaAqm[idx++][0] = mu.parameterNamed(materials[exAtM.material()].getString("k"));

    //! A vérifier
    for( auto const& exAtM : bc["temperature"]["Robin"] )
    {
        auto e = expr(exAtM.expression1());
        for( auto const& param : M_modelProps->parameters() )
            if( e.expression().hasSymbol(param.first) )
                e.setParameterValues( { param.first, mu.parameterNamed(param.first) } );

        M_betaAqm[idx++][0] = e.evaluate();
    }

    for( int m = 0; m < eimGradGrad->mMax(); ++m )
        M_betaFqm[0][0][m] = betaEimGradGrad(m);
    idx = 1;


    //! A vérifier
    for( auto const& exAtM : bc["potential"]["Dirichlet"] )
    {
        auto e = expr(exAtM.expression());
        for( auto const& param : M_modelProps->parameters() )
            if( e.expression().hasSymbol(param.first) )
                e.setParameterValues( { param.first, mu.parameterNamed(param.first) } );

        M_betaFqm[0][idx++][0] = mu.parameterNamed(materials[exAtM.material()].getString("sigma"))*e.evaluate();
    }

    for( auto const& exAtM : bc["temperature"]["Dirichlet"] )
    {
        auto e = expr(exAtM.expression());
        for( auto const& param : M_modelProps->parameters() )
            if( e.expression().hasSymbol(param.first) )
                e.setParameterValues( { param.first, mu.parameterNamed(param.first) } );

        M_betaFqm[0][idx++][0] = mu.parameterNamed(materials[exAtM.material()].getString("k"))*e.evaluate();
    }


    for( auto const& exAtM : bc["temperature"]["Robin"] )
    {
        auto e = expr(exAtM.expression2());
        for( auto const& param : M_modelProps->parameters() )
            if( e.expression().hasSymbol(param.first) )
                e.setParameterValues( { param.first, mu.parameterNamed(param.first) } );

        M_betaFqm[0][idx++][0] = e.evaluate();
    }
}

Thermoelectric::beta_vector_type
Thermoelectric::computeBetaLinearDecompositionA( parameter_type const& mu, double time )
{
    beta_vector_type beta;
    return beta;
}

Thermoelectric::affine_decomposition_type
Thermoelectric::computeAffineDecomposition()
{
    return boost::make_tuple( this->M_Aqm, this->M_Fqm);
}

std::vector<std::vector<Thermoelectric::sparse_matrix_ptrtype> >
Thermoelectric::computeLinearDecompositionA()
{
    return this->M_linearAqm;
}

std::vector<std::vector<Thermoelectric::element_ptrtype> >
Thermoelectric::computeInitialGuessAffineDecomposition()
{
    return M_InitialGuess;
}

Thermoelectric::element_type
Thermoelectric::solve( parameter_type const& mu )
{
    Feel::cout << "solve for parameter:" << std::endl << mu << std::endl;
    auto Vh = Xh->template functionSpace<0>();
    auto Th = Xh->template functionSpace<1>();
    auto V = Vh->element();
    auto phiV = Vh->element();
    auto T = Th->element();
    auto phiT = Th->element();

    auto bc = M_modelProps->boundaryConditions();
    auto materials = M_modelProps->materials();
    auto gamma = doption("thermoelectric.gamma");

    // Surface de Gamma In
    auto S_gammaIn = doption("S_gammaIn");

    // Marker du mesh sur lequel on veut rajouter W
    auto W_marker = soption( "W_marker");

    auto I = mu.parameterNamed("I");
    auto h = mu.parameterNamed("h");
    auto T_ext = mu.parameterNamed("T_ext");

    // Puissance suplémentaire qu'on veut rajouter sur une partie du système
    auto W = mu.parameterNamed("W");


    std::map<std::string , double> sigma;
    std::map<std::string , double> k;


    for (auto const& mat : materials){

        sigma[mat.marker()]=mu.parameterNamed(mat.getString("sigma"));
        k[mat.marker()]=mu.parameterNamed(mat.getString("k"));
    }


    /***************************** Electro *****************************/
    tic();
    auto a = form2(_test=Vh, _trial=Vh);
    // V
    int i = 0;
    for (auto const& mat: materials){
        a = integrate( markedelements(M_mesh,mat.marker()),
                       sigma[mat.marker()]*inner(gradt(V),grad(phiV)) );
    }

    // V Dirichlet condition

    for( auto const& exAtM : bc["potential"]["Dirichlet"] )
    {

        a += integrate( markedfaces(M_mesh, exAtM.marker() ),
                        sigma[exAtM.material()]*(gamma/hFace()*inner(idt(V),id(phiV))
                               -inner(gradt(V)*N(),id(phiV))
                               -inner(grad(phiV)*N(),idt(V)) ) );
    }


    // V Neumann

    for (auto const& exAtM : bc["potential"]["Neumann"])
    {
        a += integrate(markedfaces(M_mesh, exAtM.marker()),
                        -I/S_gammaIn*id(phiV));
    }

    auto f = form1(_test=Vh);

    // V Dirichlet condition

    //! A vérifier
    for( auto const& exAtM : bc["potential"]["Dirichlet"] )
    {
        auto e = expr(exAtM.expression());
        for( auto const& param : M_modelProps->parameters() )
            if( e.expression().hasSymbol(param.first) )
                e.setParameterValues( { param.first, mu.parameterNamed(param.first) } );

        f += integrate( markedfaces(M_mesh, exAtM.marker() ),
                        sigma[exAtM.material()]*e*(gamma/hFace()*id(phiV) -  grad(phiV)*N()) );
    }




    a.solve( _solution=V, _rhs=f, _name="mono" );

    auto aT = form2(_test=Th, _trial=Th);
    // T

    i = 0;
    for (auto const& mat : materials){
        aT += integrate( markedelements(M_mesh, mat.marker()),
                         k[mat.marker()]*inner(gradt(T), grad(phiT)) );
    }

    // T Dirichlet
    for( auto const& exAtM : bc["temperature"]["Dirichlet"] )
    {

        aT += integrate( markedfaces(M_mesh, exAtM.marker() ),
                        k[exAtM.material()]*(gamma/hFace()*inner(idt(T),id(phiT))
                                                 -inner(gradt(T)*N(),id(phiT))
                                                 -inner(grad(phiT)*N(),idt(T)) ) );
    }

    // T Robin condition
    for( auto const& exAtM : bc["temperature"]["Robin"] )
    {
        auto e = expr(exAtM.expression1());
        for( auto const& param : M_modelProps->parameters() )
            if( e.expression().hasSymbol(param.first) )
                e.setParameterValues( { param.first, mu.parameterNamed(param.first) } );

        aT += integrate( markedfaces(M_mesh, exAtM.marker() ),
                         e*inner(idt(T), id(phiT)) );
    }

    auto fT = form1(_test=Th);
    // T right hand side

    i = 0;

    for (auto const& mat : materials){
        fT = integrate(markedelements(M_mesh, mat.marker()),
                       id(phiT)*sigma[mat.marker()]*gradv(V)*trans(gradv(V)) );
    }

    // Condition Dirichlet pour T
    for( auto const& exAtM : bc["temperature"]["Dirichlet"] )
    {
        auto e = expr(exAtM.expression());
        for( auto const& param : M_modelProps->parameters() )
            if( e.expression().hasSymbol(param.first) )
                e.setParameterValues( { param.first, mu.parameterNamed(param.first) } );

        fT += integrate( markedfaces(M_mesh, exAtM.marker() ),
                        k[exAtM.material()]*e*(gamma/hFace()*id(phiT) -  grad(phiT)*N()) );
    }

    // T Robin condition

    //! Sensé être les mêmes si je ne me trompe pas
    for( auto const& exAtM : bc["temperature"]["Robin"] )
    {
        auto e = expr(exAtM.expression2());
        for( auto const& param : M_modelProps->parameters() )
            if( e.expression().hasSymbol(param.first) )
                e.setParameterValues( { param.first, mu.parameterNamed(param.first) } );

        fT += integrate( markedfaces(M_mesh, exAtM.marker() ),
                         e*id(phiT) );
    }

    // Rajout de la puissance supplémentaire sur la partie marqué par W_marker

    fT += integrate(markedfaces(M_mesh, W_marker),
                    W*id(phiT));

    aT.solve( _solution=T, _rhs=fT, _name="mono" );

    auto solution = Xh->element();
    solution.template element<0>() = V;
    solution.template element<1>() = T;

    auto e = exporter(M_mesh);
    e->add("sol", solution);
    e->save();
    toc("mono");

    return solution;
}

double Thermoelectric::output( int output_index, parameter_type const& mu , element_type& u, bool need_to_solve)
{
    auto mesh = Xh->mesh();
    double output=0;
    if ( output_index == 0 )
    {
        for ( int q = 0; q < Ql(0); q++ )
        {
            element_ptrtype eltF( new element_type( Xh ) );
            *eltF = *M_Fqm[output_index][q][0];
            output += M_betaFqm[output_index][q][0]*dot( *eltF, u );
            //output += M_betaFqm[output_index][q][m]*dot( M_Fqm[output_index][q][m], U );
        }
    }
    else
        throw std::logic_error( "[Heat2d::output] error with output_index : only 0 or 1 " );
    return output;
}

int Thermoelectric::mMaxSigma()
{
    return 1;
}

Thermoelectric::q_sigma_element_type Thermoelectric::eimSigmaQ(int m)
{
    auto Vh = Xh->template functionSpace<0>();
    q_sigma_element_type q = Vh->element();
    q.on( _range=elements(M_mesh), _expr=cst(1.) );
    return q;
}

Thermoelectric::vectorN_type Thermoelectric::eimSigmaBeta( parameter_type const& mu )
{
    vectorN_type beta(1);
    beta(0) = mu.parameterNamed( "sigma" );
    return beta;
}

void Thermoelectric::computeTruthCurrentDensity( current_element_type& j, parameter_type const& mu )
{
    auto VT = this->solve(mu);
    auto V = VT.template element<0>();
    auto sigma = mu.parameterNamed("sigma_fils");
    auto Vh = j.functionSpace();
    j = vf::project(Vh, elements(M_mesh), cst(-1.)*sigma*trans(gradv(V)) );
}

FEELPP_CRB_PLUGIN( Thermoelectric, "thermoelectric")
}