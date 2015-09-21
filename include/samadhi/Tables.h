#pragma once

#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/ppgen.h>

SQLPP_DECLARE_TABLE(
    (dataset, SQLPP_CHARACTER_SET("utf-8"))
    ,
    (dataset_id     , int         , SQLPP_AUTO_INCREMENT)
    (name           , varchar(255), SQLPP_NOT_NULL      )
    (datatype       , varchar(255), SQLPP_NOT_NULL      )
    (process        , varchar(255), SQLPP_NULL          )
    (nevents        , int         , SQLPP_NULL          )
    (dsize          , bigint      , SQLPP_NULL          )
    (xsection       , float       , SQLPP_NULL          )
    (cmssw_release  , varchar(255), SQLPP_NULL          )
    (globaltag      , varchar(255), SQLPP_NULL          )
    (energy         , float       , SQLPP_NULL          )
    (creation_time  , bigint      , SQLPP_NULL          )
    (user_comment   , text        , SQLPP_NULL          )
)

SQLPP_DECLARE_TABLE(
    (sample, SQLPP_CHARACTER_SET("utf-8"))
    ,
    (sample_id        , int         , SQLPP_AUTO_INCREMENT)
    (name             , varchar(255), SQLPP_NOT_NULL      )
    (path             , varchar(255), SQLPP_NOT_NULL      )
    (sampletype       , varchar(255), SQLPP_NOT_NULL      )
    (nevents_processed, int         , SQLPP_NULL          )
    (nevents          , int         , SQLPP_NULL          )
    (normalization    , float       , SQLPP_NOT_NULL      )
    (luminosity       , float       , SQLPP_NULL          )
    (code_version     , varchar(255), SQLPP_NULL          )
    (user_comment     , text        , SQLPP_NULL          )
    (author           , text        , SQLPP_NULL          )
    (creation_time    , bigint      , SQLPP_NULL          )
    (source_dataset_id, int         , SQLPP_NOT_NULL      )
    (source_sample_id , int         , SQLPP_NOT_NULL      )
)
