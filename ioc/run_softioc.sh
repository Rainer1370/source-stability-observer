#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

limits='P=SSO:,IMAGE_NELM=16384,KV_LOLO=40,KV_LOW=45,KV_HIGH=52,KV_HIHI=55,MA_LOLO=0.14,MA_LOW=0.17,MA_HIGH=0.23,MA_HIHI=0.26,RETURN_LOW=-4,RETURN_HIGH=2,RETURN_HIHI=4,TEMP_HIGH=35,TEMP_HIHI=45,VAC_HIGH=1e-6,VAC_HIHI=1e-5,FLUX_LOLO=0.70,FLUX_LOW=0.85,FIT_LOW=0.92,FIT_LOLO=0.80'
exec softIoc -S -m "$limits" -d SourceObserverApp/Db/sourceObserver.db
