/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t  -*-

 This file is part of the Feel++ library

 Author(s): JB Wahl <wahl@math.unistra.fr>
 Date: 18 April 2017

 Copyright (C) 2016 Feel++ Consortium

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

#ifndef FEELPP_SER_HPP
#define FEELPP_SER_HPP 1

namespace Feel{

template <typename CRBType>
class SER
{
public :
    typedef CRBType crb_type;
    typedef boost::shared_ptr<crb_type> crb_ptrtype;

    typedef typename crb_type::model_type crbmodel_type;
    typedef boost::shared_ptr<crbmodel_type> crbmodel_ptrtype;

    typedef typename crbmodel_type::model_type model_type;
    typedef boost::shared_ptr<model_type> model_ptrtype;


    SER( crb_ptrtype crb, crbmodel_ptrtype crbmodel );

    void run();

    crb_ptrtype crb( int const& level )
    {
        return M_crbs[level];
    }

private :
    std::vector<crbmodel_ptrtype> M_models;
    std::vector<crb_ptrtype> M_crbs;
}; //class SER


template <typename CRBType>
SER<CRBType>::SER( crb_ptrtype crb, crbmodel_ptrtype crbmodel )
{
    M_models.push_back( crbmodel );
    M_crbs.push_back( crb );
} // SER constructor


template <typename CRBType>
void
SER<CRBType>::run()
{
    bool do_offline_eim = false;
    int nb_levels = ioption( _name="ser.nb-levels" );

    for( int ser_level=0; ser_level < nb_levels; ++ser_level )
    {
        if ( ser_level > 0 ) // create new crb and model
        {
            auto model = boost::make_shared<crbmodel_type>( crb::stage::offline, ser_level );
            auto crb = boost::make_shared<crb_type>( Environment::about().appName(), model, crb::stage::offline );
            M_models.push_back( model );
            M_crbs.push_back( crb );
        }

        auto model = M_models.back();
        auto crb = M_crbs.back();

        auto eim_sc_vector = model->scalarContinuousEim();
        auto eim_sd_vector = model->scalarDiscontinuousEim();
        for( auto eim_sc : eim_sc_vector )
            do_offline_eim = do_offline_eim || eim_sc->offlineStep();
        for( auto eim_sd : eim_sd_vector )
            do_offline_eim = do_offline_eim || eim_sd->offlineStep();

        do
        {
            //Begin with rb since first eim has already been built in initModel
            //this->loadDB(); // update AffineDecomposition and enrich RB database
            tic();
            crb->setOfflineStep( true );
            do  // SER r-adaptation for RB
            {
                crb->setAdaptationSER( false ); //re-init to false
                crb->offline();
            }
            while( crb->adaptationSER() );
            toc("SER - crb offline", FLAGS_v>0);

            crb->setRebuild( false ); //do not rebuild since co-build is not finished
            int use_rb = boption(_name="ser.use-rb-in-eim-mu-selection") || boption(_name="ser.use-rb-in-eim-basis-build");

tic();
            if( do_offline_eim && crb->offlineStep() ) //Continue to enrich EIM functionspace only is RB is not complete
            {
                do_offline_eim = false; //re-init
                for( auto eim_sc : eim_sc_vector )
                {
                    eim_sc->setRestart( false ); //do not restart since co-build is not finished

                    if( use_rb )
                    {
                        if ( M_crbs.size() > 1 )
                        {
                            CHECK( M_crbs.size() == M_models.size() );
                            eim_sc->setRB( M_crbs[ser_level-1] ); //update rb model member to be used in eim offline
                            eim_sc->setModel( M_models[ser_level-1] );
                        }
                        else
                        {
                            eim_sc->setRB( crb ); // current crb (first level)
                            eim_sc->setModel( model );
                        }
                    }
                    do //r-adaptation for EIM
                    {
                        eim_sc->setAdaptationSER( false ); //re-init to false
                        eim_sc->offline();
                    } while( eim_sc->adaptationSER() );

                    do_offline_eim = do_offline_eim || eim_sc->offlineStep();
                }
                for( auto eim_sd : eim_sd_vector )
                {
                    eim_sd->setRestart( false ); //do not restart since co-build is not finished

                    if( use_rb )
                    {
                        if ( M_crbs.size() > 1 )
                        {
                            CHECK( M_crbs.size() == M_models.size() );
                            eim_sd->setRB( M_crbs[ser_level-1] ); //update rb model member to be used in eim offline
                            eim_sd->setModel( M_models[ser_level-1] );
                        }
                        else
                        {
                            eim_sd->setRB( crb ); //update rb model member to be used in eim offline
                            eim_sd->setModel( model );
                        }
                    }
                    do //r-adaptation for EIM
                    {
                        eim_sd->setAdaptationSER( false ); //re-init to false
                        eim_sd->offline();
                    } while( eim_sd->adaptationSER() );

                    do_offline_eim = do_offline_eim || eim_sd->offlineStep();
                }

                model->assemble(); //Affine decomposition has changed since eim has changed
            }
            toc("SER - eim offline + re-assemble", FLAGS_v>0);

        } while( crb->offlineStep() );

    } // for( int ser_level=0; ser_level < nb_levels; ++ser_level )


} // run


} //namespace Feel

#endif // FEELPP_SER_HPP