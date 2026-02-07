/*
 * libpoporon - poporon.c
 *
 * This file is part of libpoporon.
 *
 * Author: Go Kudo <zeriyoshi@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <poporon.h>

#include "internal/common.h"
#include "internal/config.h"
#include "internal/ldpc.h"

static inline void decoder_buffer_destroy(decoder_buffer_t *buffer)
{
    pfree(buffer->error_locator);
    pfree(buffer);
}

static inline decoder_buffer_t *decoder_buffer_create(uint16_t num_roots)
{
    decoder_buffer_t *buffer;
    uint16_t *raw_buffer;
    uint32_t i;

    buffer = (decoder_buffer_t *)pmalloc(sizeof(decoder_buffer_t));
    if (!buffer) {
        return NULL;
    }

    raw_buffer = (uint16_t *)pmalloc(8 * (num_roots + 1) * sizeof(uint16_t));
    if (!raw_buffer) {
        pfree(buffer);

        return NULL;
    }

    buffer->primitive_inverse = 0;
    buffer->error_locator = raw_buffer;
    buffer->syndrome = buffer->error_locator + (num_roots + 1);
    buffer->coefficients = buffer->syndrome + (num_roots + 1);
    buffer->polynomial = buffer->coefficients + (num_roots + 1);
    buffer->error_evaluator = buffer->polynomial + (num_roots + 1);
    buffer->error_roots = buffer->error_evaluator + (num_roots + 1);
    buffer->register_coefficients = buffer->error_roots + (num_roots + 1);
    buffer->error_locations = buffer->register_coefficients + (num_roots + 1);

    for (i = 0; i < 8 * (num_roots + 1); i++) {
        raw_buffer[i] = 0;
    }

    return buffer;
}

static inline poporon_t *poporon_create_rs_internal(const poporon_config_t *cfg)
{
    poporon_t *pprn;
    poporon_rs_t *rs;
    decoder_buffer_t *buffer;
    uint32_t iterations;
    uint16_t primitive_inverse;

    rs = poporon_rs_create(cfg->params.rs.symbol_size, cfg->params.rs.generator_polynomial,
                           cfg->params.rs.first_consecutive_root, cfg->params.rs.primitive_element,
                           cfg->params.rs.num_roots);
    if (!rs) {
        return NULL;
    }

    buffer = decoder_buffer_create(cfg->params.rs.num_roots);
    if (!buffer) {
        poporon_rs_destroy(rs);
        return NULL;
    }

    if (cfg->params.rs.primitive_element == 0) {
        decoder_buffer_destroy(buffer);
        poporon_rs_destroy(rs);
        return NULL;
    }

    iterations = 0;
    for (primitive_inverse = 1; (primitive_inverse % cfg->params.rs.primitive_element) != 0;
         primitive_inverse += rs->gf->field_size) {
        if (++iterations > rs->gf->field_size * 2) {
            decoder_buffer_destroy(buffer);
            poporon_rs_destroy(rs);
            return NULL;
        }
    }
    buffer->primitive_inverse = primitive_inverse / cfg->params.rs.primitive_element;

    pprn = (poporon_t *)pcalloc(1, sizeof(poporon_t));
    if (!pprn) {
        decoder_buffer_destroy(buffer);
        poporon_rs_destroy(rs);
        return NULL;
    }

    pprn->fec_type = PPLN_FEC_RS;
    pprn->ctx.rs.rs = rs;
    pprn->ctx.rs.buffer = buffer;
    pprn->ctx.rs.erasure = cfg->params.rs.erasure;
    pprn->ctx.rs.ext_syndrome = cfg->params.rs.syndrome;
    pprn->ctx.rs.last_corrected = 0;

    return pprn;
}

static inline poporon_t *poporon_create_ldpc_internal(const poporon_config_t *cfg)
{
    poporon_t *pprn;
    poporon_ldpc_params_t ldpc_params;
    poporon_ldpc_t *ldpc;

    ldpc_params.matrix_type = cfg->params.ldpc.matrix_type;
    ldpc_params.column_weight = cfg->params.ldpc.column_weight;
    ldpc_params.use_inner_interleave = cfg->params.ldpc.use_inner_interleave;
    ldpc_params.use_outer_interleave = cfg->params.ldpc.use_outer_interleave;
    ldpc_params.interleave_depth = cfg->params.ldpc.interleave_depth;
    ldpc_params.lifting_factor = cfg->params.ldpc.lifting_factor;
    ldpc_params.seed = cfg->params.ldpc.seed;

    ldpc = poporon_ldpc_create(cfg->params.ldpc.block_size, cfg->params.ldpc.rate, &ldpc_params);
    if (!ldpc) {
        return NULL;
    }

    pprn = (poporon_t *)pcalloc(1, sizeof(poporon_t));
    if (!pprn) {
        poporon_ldpc_destroy(ldpc);
        return NULL;
    }

    pprn->fec_type = PPLN_FEC_LDPC;
    pprn->ctx.ldpc.ldpc = ldpc;
    pprn->ctx.ldpc.soft_llr = cfg->params.ldpc.soft_llr;
    pprn->ctx.ldpc.soft_llr_size = cfg->params.ldpc.soft_llr_size;
    pprn->ctx.ldpc.use_soft_decode = cfg->params.ldpc.use_soft_decode;
    pprn->ctx.ldpc.max_iterations = cfg->params.ldpc.max_iterations;
    pprn->ctx.ldpc.last_iterations = 0;

    return pprn;
}

static inline poporon_t *poporon_create_bch_internal(const poporon_config_t *cfg)
{
    poporon_t *pprn;
    poporon_bch_t *bch;

    bch = poporon_bch_create(cfg->params.bch.symbol_size, cfg->params.bch.generator_polynomial,
                             cfg->params.bch.correction_capability);
    if (!bch) {
        return NULL;
    }

    pprn = (poporon_t *)pcalloc(1, sizeof(poporon_t));
    if (!pprn) {
        poporon_bch_destroy(bch);
        return NULL;
    }

    pprn->fec_type = PPLN_FEC_BCH;
    pprn->ctx.bch.bch = bch;
    pprn->ctx.bch.last_num_errors = 0;

    return pprn;
}

extern poporon_t *poporon_create(const poporon_config_t *config)
{
    if (!config) {
        return NULL;
    }

    switch (config->fec_type) {
    case PPLN_FEC_RS:
        return poporon_create_rs_internal(config);
    case PPLN_FEC_LDPC:
        return poporon_create_ldpc_internal(config);
    case PPLN_FEC_BCH:
        return poporon_create_bch_internal(config);
    default:
        return NULL;
    }
}

extern void poporon_destroy(poporon_t *pprn)
{
    if (!pprn) {
        return;
    }

    switch (pprn->fec_type) {
    case PPLN_FEC_RS:
        if (pprn->ctx.rs.buffer) {
            decoder_buffer_destroy(pprn->ctx.rs.buffer);
        }
        poporon_rs_destroy(pprn->ctx.rs.rs);
        break;
    case PPLN_FEC_LDPC:
        poporon_ldpc_destroy(pprn->ctx.ldpc.ldpc);
        break;
    case PPLN_FEC_BCH:
        poporon_bch_destroy(pprn->ctx.bch.bch);
        break;
    }

    pfree(pprn);
}

extern poporon_config_t *poporon_rs_config_create(uint8_t symbol_size, uint16_t generator_polynomial,
                                                  uint16_t first_consecutive_root, uint16_t primitive_element,
                                                  uint8_t num_roots, poporon_erasure_t *erasure, uint16_t *syndrome)
{
    poporon_config_t *config = (poporon_config_t *)pcalloc(1, sizeof(poporon_config_t));
    if (!config) {
        return NULL;
    }

    config->fec_type = PPLN_FEC_RS;
    config->params.rs.symbol_size = symbol_size;
    config->params.rs.generator_polynomial = generator_polynomial;
    config->params.rs.first_consecutive_root = first_consecutive_root;
    config->params.rs.primitive_element = primitive_element;
    config->params.rs.num_roots = num_roots;
    config->params.rs.erasure = erasure;
    config->params.rs.syndrome = syndrome;

    return config;
}

extern poporon_config_t *poporon_ldpc_config_create(size_t block_size, poporon_ldpc_rate_t rate,
                                                    poporon_ldpc_matrix_type_t matrix_type, uint32_t column_weight,
                                                    bool use_soft_decode, bool use_outer_interleave,
                                                    bool use_inner_interleave, uint32_t interleave_depth,
                                                    uint32_t lifting_factor, uint32_t max_iterations,
                                                    const int8_t *soft_llr, size_t soft_llr_size, uint64_t seed)
{
    poporon_config_t *config = (poporon_config_t *)pcalloc(1, sizeof(poporon_config_t));
    if (!config) {
        return NULL;
    }

    config->fec_type = PPLN_FEC_LDPC;
    config->params.ldpc.block_size = block_size;
    config->params.ldpc.rate = rate;
    config->params.ldpc.matrix_type = matrix_type;
    config->params.ldpc.column_weight = column_weight;
    config->params.ldpc.use_soft_decode = use_soft_decode;
    config->params.ldpc.use_outer_interleave = use_outer_interleave;
    config->params.ldpc.use_inner_interleave = use_inner_interleave;
    config->params.ldpc.interleave_depth = interleave_depth;
    config->params.ldpc.lifting_factor = lifting_factor;
    config->params.ldpc.max_iterations = max_iterations;
    config->params.ldpc.soft_llr = soft_llr;
    config->params.ldpc.soft_llr_size = soft_llr_size;
    config->params.ldpc.seed = seed;

    return config;
}

extern poporon_config_t *poporon_bch_config_create(uint8_t symbol_size, uint16_t generator_polynomial,
                                                   uint8_t correction_capability)
{
    poporon_config_t *config = (poporon_config_t *)pcalloc(1, sizeof(poporon_config_t));
    if (!config) {
        return NULL;
    }

    config->fec_type = PPLN_FEC_BCH;
    config->params.bch.symbol_size = symbol_size;
    config->params.bch.generator_polynomial = generator_polynomial;
    config->params.bch.correction_capability = correction_capability;

    return config;
}

extern poporon_config_t *poporon_config_rs_default(void)
{
    return poporon_rs_config_create(8, 0x11D, 1, 1, 32, NULL, NULL);
}

extern poporon_config_t *poporon_config_ldpc_default(size_t block_size, poporon_ldpc_rate_t rate)
{
    return poporon_ldpc_config_create(block_size, rate, PPRN_LDPC_RANDOM, 3, true, true, true, 0, 0, 0, NULL, 0, 0);
}

extern poporon_config_t *poporon_config_ldpc_burst_resistant(size_t block_size, poporon_ldpc_rate_t rate)
{
    return poporon_ldpc_config_create(block_size, rate, PPRN_LDPC_RANDOM, 7, true, true, true, 0, 0, 0, NULL, 0, 0);
}

extern poporon_config_t *poporon_config_bch_default(void)
{
    return poporon_bch_config_create(4, 0x13, 3);
}

extern void poporon_config_destroy(poporon_config_t *config)
{
    pfree(config);
}

extern poporon_fec_type_t poporon_get_fec_type(const poporon_t *pprn)
{
    if (!pprn) {
        return PPLN_FEC_UNKNOWN;
    }

    return pprn->fec_type;
}

extern uint32_t poporon_get_iterations_used(const poporon_t *pprn)
{
    if (!pprn || pprn->fec_type != PPLN_FEC_LDPC) {
        return 0;
    }

    return pprn->ctx.ldpc.last_iterations;
}

extern size_t poporon_get_parity_size(const poporon_t *pprn)
{
    uint16_t cw, data;

    if (!pprn) {
        return 0;
    }

    switch (pprn->fec_type) {
    case PPLN_FEC_RS:
        return pprn->ctx.rs.rs->num_roots;
    case PPLN_FEC_LDPC:
        return poporon_ldpc_parity_size(pprn->ctx.ldpc.ldpc);
    case PPLN_FEC_BCH: {
        cw = poporon_bch_get_codeword_length(pprn->ctx.bch.bch);
        data = poporon_bch_get_data_length(pprn->ctx.bch.bch);
        return (size_t)(cw - data + 7) / 8;
    }
    default:
        return 0;
    }
}

extern size_t poporon_get_info_size(const poporon_t *pprn)
{
    if (!pprn) {
        return 0;
    }

    switch (pprn->fec_type) {
    case PPLN_FEC_RS:
        return (size_t)(pprn->ctx.rs.rs->gf->field_size - pprn->ctx.rs.rs->num_roots);
    case PPLN_FEC_LDPC:
        return poporon_ldpc_info_size(pprn->ctx.ldpc.ldpc);
    case PPLN_FEC_BCH:
        return (size_t)(poporon_bch_get_data_length(pprn->ctx.bch.bch) + 7) / 8;
    default:
        return 0;
    }
}

extern uint32_t poporon_version_id()
{
    return (uint32_t)POPORON_VERSION_ID;
}

extern poporon_buildtime_t poporon_buildtime()
{
    return (poporon_buildtime_t)POPORON_BUILDTIME;
}
