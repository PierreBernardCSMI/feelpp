/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t -*-

  This file is part of the Feel library

  Author(s): Christophe Prud'homme <christophe.prudhomme@feelpp.org>
       Date: 2014-02-14

  Copyright (C) 2014-2016 Feel++ Consortium

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#if !defined( FEELPP_VF_DETAIL_GINACEXVF_HPP )
#define FEELPP_VF_DETAIL_GINACEXVF_HPP 1

namespace Feel{
namespace vf{

/**
 * \class Ginac
 * \brief allow runtime ginac in expression
 *
 * @author Christophe Prud'homme
 * @see
 */
template<int Order=2, typename SymbolsExprType = SymbolsExpr<> >
class FEELPP_EXPORT GinacExVF : public Feel::vf::GiNaCBase
{
public:


    /** @name Typedefs
     */
    //@{
    typedef Feel::vf::GiNaCBase super;

    typedef SymbolsExprType symbols_expression_type;
    typedef typename symbols_expression_type::tuple_type symbols_expression_tuple_type;

    struct FunctorsVariadicExpr
    {
        struct Context
        {
            template <typename T1,typename T2>
            constexpr auto operator()( T1 const& res,T2 const& e ) const
                {
                    return hana::integral_constant<size_type, T1::value | std::tuple_element<1,typename T2::value_type>::type::context >{};
                }
        };
        template<typename Funct>
        struct HasTestFunction
        {
            template <typename T1,typename T2>
            constexpr auto operator()( T1 const& res,T2 const& e ) const
                {
                    return hana::integral_constant<bool, T1::value || std::tuple_element<1,typename T2::value_type>::type::template HasTestFunction<Funct>::result >{};
                }
        };
        template<typename Funct>
        struct HasTrialFunction
        {
            template <typename T1,typename T2>
            constexpr auto operator()( T1 const& res,T2 const& e ) const
                {
                    return hana::integral_constant<bool, T1::value || std::tuple_element<1,typename T2::value_type>::type::template HasTrialFunction<Funct>::result >{};
                }
        };
        template<typename Funct>
        struct HasTestBasis
        {
            template <typename T1,typename T2>
            constexpr auto operator()( T1 const& res,T2 const& e ) const
                {
                    return hana::integral_constant<bool, T1::value || std::tuple_element<1,typename T2::value_type>::type::template has_test_basis<Funct>::result >{};
                }
        };
        template<typename Funct>
        struct HasTrialBasis
        {
            template <typename T1,typename T2>
            constexpr auto operator()( T1 const& res,T2 const& e ) const
                {
                    return hana::integral_constant<bool, T1::value || std::tuple_element<1,typename T2::value_type>::type::template has_trial_basis<Funct>::result >{};
                }
        };

    };
    //static const size_type context = vm::POINT|expression_type::context;
    //static const size_type context =  std::decay_t<decltype( hana::fold( symbols_expression_tuple_type{}, hana::integral_constant<size_type,vm::POINT|vm::JACOBIAN|vm::KB|vm::NORMAL>{}, typename FunctorsVariadicExpr::Context{} ) )>::value;
    static const size_type context =  std::decay_t<decltype( hana::fold( symbols_expression_tuple_type{}, hana::integral_constant<size_type,vm::DYNAMIC>{}, typename FunctorsVariadicExpr::Context{} ) )>::value;

    static const bool is_terminal = false;

    template<typename Funct>
    struct HasTestFunction
    {
        static const bool result =  std::decay_t<decltype( hana::fold( symbols_expression_tuple_type{},
                                                                       hana::integral_constant<bool,false>{},
                                                                       typename FunctorsVariadicExpr::template HasTestFunction<Funct>{} ) )>::value;
    };
    template<typename Funct>
    struct HasTrialFunction
    {
        static const bool result =  std::decay_t<decltype( hana::fold( symbols_expression_tuple_type{},
                                                                       hana::integral_constant<bool,false>{},
                                                                       typename FunctorsVariadicExpr::template HasTrialFunction<Funct>{} ) )>::value;
    };

    template<typename Funct>
    static const bool has_test_basis = std::decay_t<decltype( hana::fold( symbols_expression_tuple_type{},
                                                                          hana::integral_constant<bool,false>{},
                                                                          typename FunctorsVariadicExpr::template HasTestBasis<Funct>{} ) )>::value;
    template<typename Funct>
    static const bool has_trial_basis = std::decay_t<decltype( hana::fold( symbols_expression_tuple_type{},
                                                                           hana::integral_constant<bool,false>{},
                                                                           typename FunctorsVariadicExpr::template HasTrialBasis<Funct>{} ) )>::value;
    using test_basis = std::nullptr_t;//TODO//typename expression_type::test_basis;
    using trial_basis = std::nullptr_t;//TODO//typename expression_type::trial_basis;

    typedef GiNaC::ex ginac_expression_type;
    typedef GinacExVF<Order,SymbolsExprType> this_type;
    typedef double value_type;
    using evaluate_type = Eigen::Matrix<value_type,1,1>;

    typedef Eigen::Matrix<value_type,Eigen::Dynamic,1> vec_type;

    template<typename... TheExpr>
    struct Lambda
    {
        typedef this_type type;
    };
    template<typename... TheExpr>
    typename Lambda<TheExpr...>::type
    operator()( TheExpr... e  ) { return *this; }

    //@}

    /** @name Constructors, destructor
     */
    //@{
    GinacExVF() : super(){}

    explicit GinacExVF( value_type value )
        :
        super(),
        M_fun( value ),
        M_cfun( new GiNaC::FUNCP_CUBA() ),
        M_exprDesc( std::to_string( value ) ),
        M_isPolynomial( true ),
        M_polynomialOrder( 0 ),
        M_isNumericExpression( true ),
        M_numericValue( value )
        {}

    explicit GinacExVF( ginac_expression_type const & fun,
                        std::vector<GiNaC::symbol> const& syms,
                        std::string const& exprDesc,
                        std::string filename="",
                        WorldComm const& world=Environment::worldComm(),
                        std::string const& dirLibExpr=Environment::exprRepository(),
                        symbols_expression_type const& expr = symbols_expression_type() )
        :
        super( syms ),
        M_fun( fun ),
        M_cfun( new GiNaC::FUNCP_CUBA() ),
        M_filename(),
        M_exprDesc( exprDesc ),
        M_expr( expr ),
        M_isPolynomial( false ),
        M_polynomialOrder( Order ),
        M_isNumericExpression( false ),
        M_numericValue( 0 )
        {
            this->updateNumericExpression();

            if ( !M_isNumericExpression )
            {
                std::string filenameExpanded = Environment::expand( filename );
                M_filename = (filenameExpanded.empty() || fs::path(filenameExpanded).is_absolute())? filenameExpanded : (fs::path(Environment::exprRepository())/filenameExpanded).string();

                DVLOG(2) << "Ginac constructor with expression_type \n";
                GiNaC::lst exprs({fun});
                GiNaC::lst syml;
                std::for_each( M_syms.begin(),M_syms.end(), [&]( GiNaC::symbol const& s ) { syml.append(s); } );

                // get filename if not given
                if ( M_filename.empty() && !M_exprDesc.empty() )
                {
                    M_filename = Feel::vf::detail::ginacGetDefaultFileName( M_exprDesc, dirLibExpr );
                }

                // build ginac lib and link if necessary
                Feel::vf::detail::ginacBuildLibrary( exprs, syml, M_exprDesc, M_filename, world, M_cfun );
            }

            this->updateForUse();
        }

    explicit GinacExVF( ginac_expression_type const & fun,
                        std::vector<GiNaC::symbol> const& syms,
                        GiNaC::FUNCP_CUBA const& cfun,
                        std::string const& exprDesc,
                        symbols_expression_type const& expr )
        :
        super( syms ),
        M_fun( fun ),
        M_cfun( new GiNaC::FUNCP_CUBA( cfun ) ),
        M_exprDesc( exprDesc ),
        M_expr( expr ),
        M_isPolynomial( false ),
        M_polynomialOrder( Order ),
        M_isNumericExpression( false ),
        M_numericValue( 0 )
        {
            this->updateNumericExpression();
            this->updateForUse();
        }

    GinacExVF( GinacExVF const & fun ) = default;
    GinacExVF( GinacExVF && fun ) = default;

    //@}

    /** @name Operator overloads
     */
    //@{

    this_type& operator=( this_type const& ) = default;
    this_type& operator=( this_type && ) = default;

    //@}

    /** @name Accessors
     */
    //@{

    //! polynomial order
    uint16_type polynomialOrder() const { return M_polynomialOrder; }

    //! expression is polynomial?
    bool isPolynomial() const { return M_isPolynomial; }

    ginac_expression_type const& expression() const
    {
        return M_fun;
    }

    const GiNaC::FUNCP_CUBA& fun() const
    {
        return *M_cfun;
    }

    //@}

    /** @name  Mutators
     */
    //@{


    //@}

    /** @name  Methods
     */
    //@{

    bool isConstant() const
    {
        return M_isNumericExpression || ( M_indexSymbolXYZ.empty() && M_indexSymbolN.empty() && (M_syms.size() == M_symbolNameToValue.size()) );
    }

    uint16_type index( std::string const& sname ) const
    {
        auto it = std::find_if( M_syms.begin(), M_syms.end(),
                                [=]( GiNaC::symbol const& s ) { return s.get_name() == sname; } );
        if ( it != M_syms.end() )
            {
                return it-M_syms.begin();
            }
        return invalid_uint16_type_value;
    }

    const std::vector<std::vector<std::tuple<uint16_type,uint16_type,uint16_type> > > indices() const
    {
        std::vector<std::vector<std::tuple<uint16_type,uint16_type,uint16_type> > > indices_vec;

        hana::for_each( M_expr.tupleExpr, [&]( auto const& evec )
                        {
                            for ( auto const& e : evec )
                            {
                                std::vector<std::tuple<uint16_type,uint16_type,uint16_type> > tmp;
                                if ( std::get<2>( e ).empty() ) // no suffix comp
                                {
                                    uint16_type idx = this->index( std::get<0>( e ) );
                                    if ( idx != invalid_v<uint16_type> )
                                        tmp.push_back( std::make_tuple( idx, 0, 0 ) );
                                }
                                else
                                {
                                    for ( auto const& [_suffix,compArray] : std::get<2>( e ) )
                                    {
                                        uint16_type idx = this->index( std::get<0>( e ) + _suffix );
                                        if ( idx != invalid_v<uint16_type> )
                                        {
                                            uint16_type c1 = compArray[0];
                                            uint16_type c2 = compArray[1];
                                            tmp.push_back( std::make_tuple( idx, c1, c2 ) );
                                        }
                                    }
                                }
                                indices_vec.push_back( tmp );
                            }
                        });
        return indices_vec;
    }
    bool isZero() const { return M_isNumericExpression? (M_numericValue == 0) : M_fun.is_zero(); }
    std::vector<GiNaC::symbol> const& syms() const { return M_syms; }

    std::string const& exprDesc() const { return M_exprDesc; }

    symbols_expression_type const& symbolsExpression() const { return M_expr; }
    //@}


    template<typename Geo_t, typename Basis_i_t, typename Basis_j_t>
    struct tensor
    {
        struct TransformExprToTensor
        {
            template <typename T>
            struct apply {
                using type = typename std::tuple_element<1,typename T::value_type>::type::template tensor<Geo_t, Basis_i_t, Basis_j_t>;
            };

            template <typename T>
            constexpr auto operator()(T const& t) const
                {
                    using _tensor_type = typename TransformExprToTensor::template apply<T>::type;
                    std::vector<_tensor_type> res;
                    for ( auto const& sub : t )
                        res.push_back( _tensor_type( std::get<1>( sub ),Geo_t{} ) );
                    return res;
                }
            template <typename T>
            constexpr auto operator()(T const& t, Geo_t const& geom, Basis_i_t const& fev, Basis_j_t const& feu ) const
                {
                    using _tensor_type = typename TransformExprToTensor::template apply<T>::type;
                    std::vector<_tensor_type> res;
                    for ( auto const& sub : t )
                        res.push_back( _tensor_type( std::get<1>(sub),geom,fev,feu ) );
                    return res;
                }
            template <typename T>
            constexpr auto operator()(T const& t, Geo_t const& geom, Basis_i_t const& fev ) const
                {
                    using _tensor_type = typename TransformExprToTensor::template apply<T>::type;
                    std::vector<_tensor_type> res;
                    for ( auto const& sub : t )
                        res.push_back( _tensor_type( std::get<1>(sub),geom,fev ) );
                    return res;
                }
            template <typename T>
            constexpr auto operator()(T const& t, Geo_t const& geom ) const
                {
                    using _tensor_type = typename TransformExprToTensor::template apply<T>::type;
                    std::vector<_tensor_type> res;
                    for ( auto const& sub : t )
                        res.push_back( _tensor_type( std::get<1>(sub),geom ) );
                    return res;
                }
        };

        using tuple_tensor_expr_type = std::decay_t<decltype( hana::transform( symbols_expression_tuple_type{}, TransformExprToTensor{} ) ) >;

        typedef double value_type;

        using key_type = key_t<Geo_t>;
        typedef typename fusion::result_of::value_at_key<Geo_t,key_type>::type::element_type* gmc_ptrtype;
        typedef typename fusion::result_of::value_at_key<Geo_t,key_type>::type::element_type gmc_type;
        // change 0 into rank
        typedef typename mpl::if_<mpl::equal_to<mpl::int_<0>,mpl::int_<0> >,
                                  mpl::identity<Shape<gmc_type::nDim, Scalar, false, false> >,
                                  typename mpl::if_<mpl::equal_to<mpl::int_<0>,mpl::int_<1> >,
                                                    mpl::identity<Shape<gmc_type::nDim, Vectorial, false, false> >,
                                                    mpl::identity<Shape<gmc_type::nDim, Tensor2, false, false> > >::type >::type::type shape;

        typedef Eigen::Matrix<value_type,Eigen::Dynamic,1> vec_type;

        struct is_zero
        {
            static const bool value = false;
        };

        tensor( this_type const& expr,
                Geo_t const& geom, Basis_i_t const& fev, Basis_j_t const& feu )
            :
            M_expr( expr ),
            M_fun( expr.fun() ),
            M_is_zero( expr.isZero() ),
            M_is_constant( expr.isConstant() ),
            M_t_expr( hana::transform( expr.symbolsExpression().tupleExpr, [&geom,&fev,&feu](auto const& t) { return TransformExprToTensor{}(t,geom,fev,feu); } ) ),
            M_t_expr_index( expr.indices() ),
            M_gmc( fusion::at_key<key_type>( geom ).get() ),
            M_nsyms( expr.syms().size() ),
            M_y( vec_type::Zero(M_gmc->nPoints()) ),
            M_x( expr.parameterValue() ),
            M_yConstant( (M_is_constant)? expr.evaluate()(0,0) : value_type(0) )
            {}

        tensor( this_type const& expr,
                Geo_t const& geom, Basis_i_t const& fev )
            :
            M_expr( expr ),
            M_fun( expr.fun() ),
            M_is_zero( expr.isZero() ),
            M_is_constant( expr.isConstant() ),
            M_t_expr( hana::transform( expr.symbolsExpression().tupleExpr, [&geom,&fev](auto const& t) { return TransformExprToTensor{}(t,geom,fev); } ) ),
            M_t_expr_index( expr.indices() ),
            M_gmc( fusion::at_key<key_type>( geom ).get() ),
            M_nsyms( expr.syms().size() ),
            M_y( vec_type::Zero(M_gmc->nPoints()) ),
            M_x(  expr.parameterValue() ),
            M_yConstant( (M_is_constant)? expr.evaluate()(0,0) : value_type(0) )
            {}

        tensor( this_type const& expr, Geo_t const& geom )
            :
            M_expr( expr ),
            M_fun( expr.fun() ),
            M_is_zero( expr.isZero() ),
            M_is_constant( expr.isConstant() ),
            M_t_expr( hana::transform( expr.symbolsExpression().tupleExpr, [&geom](auto const& t) { return TransformExprToTensor{}(t,geom); } ) ),
            M_t_expr_index( expr.indices() ),
            M_gmc( fusion::at_key<key_type>( geom ).get() ),
            M_nsyms( expr.syms().size() ),
            M_y( vec_type::Zero(M_gmc->nPoints()) ),
            M_x( expr.parameterValue() ),
            M_yConstant( (M_is_constant)? expr.evaluate()(0,0) : value_type(0) )
            {}

        template<typename IM>
        void init( IM const& im )
        {
            uint16_type k=0;
            hana::for_each( M_t_expr, [&k,&im,this]( auto & evec )
                            {
                                for ( auto & e : evec )
                                {
                                    if ( !M_t_expr_index[k].empty() )
                                        e.init( im );
                                    ++k;
                                }
                            });
        }
        FEELPP_DONT_INLINE void updateFun(Geo_t const& geom )
            {
                M_gmc =  fusion::at_key<key_type>( geom ).get();

                int no = 1;
                int ni = M_nsyms;///gmc_type::nDim;

                for(int q = 0; q < M_gmc->nPoints();++q )
                {
                    for ( auto const& comp : M_expr.indexSymbolXYZ() )
                        M_x[comp.second] = M_gmc->xReal( q )[comp.first];
                    // is it called for updates on faces? need to check that...
                    for ( auto const& comp : M_expr.indexSymbolN() )
                        M_x[comp.second] = M_gmc->unitNormal( q )[comp.first-3];

                    uint16_type k=0;
                    hana::for_each( M_t_expr, [&k,&q,this]( auto const& evec )
                                    {
                                        for ( auto const& e : evec )
                                        {
                                            for ( auto const& [idx,c1,c2] : M_t_expr_index[k] )
                                                M_x[idx] = e.evalq( c1, c2, q );
                                            ++k;
                                        }
                                    });
                    M_fun(&ni,M_x.data(),&no,&M_y[q]);
                }
            }
        void update( Geo_t const& geom, Basis_i_t const& fev, Basis_j_t const& feu )
        {
            if ( M_is_zero ) return;
            if ( M_is_constant ) return;

            uint16_type k=0;
            hana::for_each( M_t_expr, [&k,&geom,&fev,&feu,this]( auto & evec )
                            {
                                for ( auto & e : evec )
                                {
                                    if ( !M_t_expr_index[k].empty() )
                                        e.update( geom,fev,feu );
                                    ++k;
                                }
                            });
            updateFun( geom );
        }
        void update( Geo_t const& geom, Basis_i_t const& fev )
            {
                if ( M_is_zero ) return;
                if ( M_is_constant ) return;

                uint16_type k=0;
                hana::for_each( M_t_expr, [&k,&geom,&fev,this]( auto & evec )
                                {
                                    for ( auto & e : evec )
                                    {
                                        if ( !M_t_expr_index[k].empty() )
                                            e.update( geom,fev );
                                        ++k;
                                    }
                                });
                updateFun( geom );
            }
        void update( Geo_t const& geom )
            {
                if ( M_is_zero ) return;
                if ( M_is_constant ) return;

                uint16_type k=0;
                hana::for_each( M_t_expr, [&k,&geom,this]( auto & evec )
                                {
                                    for ( auto & e : evec )
                                    {
                                         if ( !M_t_expr_index[k].empty() )
                                            e.update( geom );
                                        ++k;
                                    }
                                });
                updateFun( geom );
            }

        void update( Geo_t const& geom, uint16_type face )
            {
                if ( M_is_zero ) return;
                if ( M_is_constant ) return;

                uint16_type k=0;
                hana::for_each( M_t_expr, [&k,&geom,&face,this]( auto & evec )
                                {
                                    for ( auto & e : evec )
                                    {
                                         if ( !M_t_expr_index[k].empty() )
                                            e.update( geom, face );
                                        ++k;
                                    }
                                });
                updateFun( geom );
            }

        template<typename ... CTX>
        void updateContext( CTX const& ... ctx )
            {
                if ( M_is_zero ) return;
                boost::fusion::vector<CTX...> ctxvec( ctx... );
                update( boost::fusion::at_c<0>( ctxvec )->gmContext() );
            }

        value_type
        evalij( uint16_type i, uint16_type j ) const
            {
                return 0;
            }

        value_type
        evalijq( uint16_type /*i*/, uint16_type /*j*/, uint16_type c1, uint16_type c2, uint16_type q ) const
            {
                return evalq( c1,c2,q );
            }

        value_type
        evaliq( uint16_type i, uint16_type c1, uint16_type c2, uint16_type q ) const
            {
                return evalq( c1,c2,q );
            }

        value_type
        evalq( uint16_type c1, uint16_type c2, uint16_type q ) const
            {
                return ( M_is_constant )? M_yConstant : M_y[q];
            }

        this_type const& M_expr;
        GiNaC::FUNCP_CUBA M_fun;
        const bool M_is_zero;
        const bool M_is_constant;
        tuple_tensor_expr_type M_t_expr;
        const std::vector<std::vector<std::tuple<uint16_type,uint16_type,uint16_type>>> M_t_expr_index; // (id,c1,c2)
        gmc_ptrtype M_gmc;

        int M_nsyms;
        vec_type M_y;
        vec_type M_x;
        const value_type M_yConstant;
    };

    evaluate_type
    evaluate( std::map<std::string,value_type> const& mp  )
    {
        this->setParameterValues( mp );
        return this->evaluateImpl( true, Environment::worldCommPtr() );
    }

    evaluate_type
    evaluate( bool parallel = true, worldcomm_ptr_t const& worldcomm = Environment::worldCommPtr() ) const
    {
        return this->evaluateImpl( parallel, worldcomm );
    }
private :

    void updateNumericExpression()
    {
        auto resToNum = toNumericValues( M_fun );
        CHECK( resToNum.size() == 1 )  << "invalid size " << resToNum.size() << " : must be 1";
        M_isNumericExpression = resToNum[0].first;
        if ( M_isNumericExpression )
        {
            M_numericValue = resToNum[0].second;
            M_isPolynomial = 1;
            M_polynomialOrder = 0;
        }
    }
    void updateForUse()
    {
        if ( M_isNumericExpression )
            return;

        std::vector<std::pair<GiNaC::symbol,int>> symbTotalDegree;
        for ( auto const& thesymbxyz : this->indexSymbolXYZ() )
            symbTotalDegree.push_back( std::make_pair( M_syms[thesymbxyz.second], 1 ) );
        bool symbExprArePolynomials = true;
        hana::for_each( M_expr.tupleExpr, [&]( auto const& evec )
                        {
                            for ( auto const& e : evec )
                            {
                                auto const& theexpr = std::get<1>( e );
                                if ( std::get<2>( e ).empty() )
                                {
                                    uint16_type idx = this->index( std::get<0>( e ) );
                                    if ( idx == invalid_uint16_type_value )
                                        continue;
                                    if ( theexpr.isPolynomial() )
                                        symbTotalDegree.push_back( std::make_pair( M_syms[idx], theexpr.polynomialOrder() ) );
                                    else
                                        symbExprArePolynomials = false;
                                }
                                else
                                {
                                    for ( auto const& [_suffix,compArray] : std::get<2>( e ) )
                                    {
                                        uint16_type idx = this->index( std::get<0>( e ) + _suffix );
                                        if ( idx != invalid_v<uint16_type> )
                                        {
                                            if ( theexpr.isPolynomial() )
                                                symbTotalDegree.push_back( std::make_pair( M_syms[idx], theexpr.polynomialOrder() ) );
                                            else
                                                symbExprArePolynomials = false;
                                        }
                                    }
                                }
                            }
                        });

        if ( symbExprArePolynomials )
        {
            GiNaC::lst symlIsPoly;
            for ( auto const& thesymb : symbTotalDegree )
                symlIsPoly.append( thesymb.first );
            M_isPolynomial = M_fun.is_polynomial( symlIsPoly );
        }
        else
            M_isPolynomial = false;

        if ( M_isPolynomial )
            M_polynomialOrder = totalDegree( M_fun, symbTotalDegree );
        else
            M_polynomialOrder = Order;
    }

    evaluate_type
    evaluateImpl( bool parallel, worldcomm_ptr_t const& worldcomm ) const
    {
        if ( M_isNumericExpression )
            return evaluate_type::Constant( M_numericValue );
        int no = 1;
        int ni = M_syms.size();

        vec_type x( ni );
        for ( uint16_type k=0;k<ni;++k )
            x[k] = M_params[k];
        hana::for_each( M_expr.tupleExpr, [&]( auto const& evec )
                        {
                            for ( auto const& e : evec )
                            {
                                auto const& theexpr = std::get<1>( e );
                                if ( std::get<2>( e ).empty() )
                                {
                                    uint16_type idx = this->index( std::get<0>( e ) );
                                    if ( idx == invalid_v<uint16_type> )
                                        continue;
                                    x[idx] = theexpr.evaluate( parallel, worldcomm )(0,0);
                                }
                                else
                                {
                                    for ( auto const& [_suffix,compArray] : std::get<2>( e ) )
                                    {
                                        uint16_type idx = this->index( std::get<0>( e ) + _suffix );
                                        if ( idx != invalid_v<uint16_type> )
                                        {
                                            uint16_type c1 = compArray[0];
                                            uint16_type c2 = compArray[1];
                                            x[idx] = theexpr.evaluate( parallel, worldcomm )(c1,c2);
                                        }
                                    }
                                }
                            }
                        });

        value_type res;
        (*M_cfun)(&ni,x.data(),&no,&res );

        return evaluate_type::Constant( res );
    }

private:
    mutable ginac_expression_type  M_fun;
    std::shared_ptr<GiNaC::FUNCP_CUBA> M_cfun;
    std::string M_filename;
    std::string M_exprDesc;
    symbols_expression_type M_expr;
    bool M_isPolynomial;
    uint16_type M_polynomialOrder;
    bool M_isNumericExpression;
    value_type M_numericValue;
};

template<int Order,typename SymbolsExprType>
FEELPP_EXPORT std::ostream&
operator<<( std::ostream& os, GinacExVF<Order,SymbolsExprType> const& e )
{
    os << e.expression();
    return os;
}

template<int Order,typename SymbolsExprType>
FEELPP_EXPORT std::string
str( GinacExVF<Order,SymbolsExprType> && e )
{
    return str( std::forward<GinacExVF<Order,SymbolsExprType>>(e).expression() );
}
template<int Order,typename SymbolsExprType>
FEELPP_EXPORT std::string
str( GinacExVF<Order,SymbolsExprType> const& e )
{
    return str( e.expression() );
}

template<int Order = 2>
using GinacEx = GinacExVF<Order>;


}} // feel::vf

#endif
