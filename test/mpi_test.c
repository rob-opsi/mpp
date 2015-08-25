/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if defined(_WIN32)
#include "vld.h"
#endif

#define MODULE_TAG "mpi_test"

#include <string.h>
#include "rk_mpi.h"
#include "mpp_log.h"
#include "mpp_env.h"


#define MPI_DEC_LOOP_COUNT          4
#define MPI_ENC_LOOP_COUNT          4

#define MPI_STREAM_SIZE             (SZ_512K)

int main()
{
    MPP_RET ret;
    MppCtx ctx  = NULL;
    MppApi *mpi = NULL;
    MppEncConfig cfg;

    MppPacket dec_in    = NULL;
    MppFrame  dec_out   = NULL;

    MppFrame  enc_in    = NULL;
    MppPacket enc_out   = NULL;

    MpiCmd cmd          = MPP_CMD_BASE;
    MppParam param      = NULL;

    RK_S32 i;
    char *buf = NULL;
    RK_S32 size = MPI_STREAM_SIZE;

    mpp_log("mpi_test start\n");

    mpp_env_set_u32("mpi_debug", 0x1);

    buf = malloc(size);
    if (NULL == buf) {
        mpp_err("mpi_test malloc failed\n");
        goto MPP_TEST_FAILED;
    }

    mpp_log("mpi_test decoder test start\n");

    // decoder demo
    ret = mpp_init(&ctx, &mpi, MPP_CTX_DEC, MPP_VIDEO_CodingAVC);
    if (MPP_OK != ret) {
        mpp_err("mpp_init failed\n");
        goto MPP_TEST_FAILED;
    }

    // NOTE: decoder do not need control function

    ret = mpi->control(ctx, cmd, param);
    if (MPP_OK != ret) {
        mpp_err("mpi->control failed\n");
        goto MPP_TEST_FAILED;
    }

    // interface with both input and output
    for (i = 0; i < MPI_DEC_LOOP_COUNT; i++) {
        mpp_packet_init(&dec_in, buf, size);

        // TODO: read stream data to buf

        ret = mpi->decode(ctx, dec_in, &dec_out);
        if (MPP_OK != ret) {
            goto MPP_TEST_FAILED;
        }

        if (dec_out) {
            // interface may output multiple frame at one time
            do {
                MppFrame next = mpp_frame_get_next(dec_out);

                // TODO: diaplay function called here

                mpp_frame_deinit(dec_out);
                dec_out = next;
            } while (dec_out);
        }

        mpp_packet_deinit(dec_in);
        dec_in = NULL;
    }

    // interface with input and output separated
    for (i = 0; i < MPI_DEC_LOOP_COUNT; i++) {
        mpp_packet_init(&dec_in, buf, size);

        // TODO: read stream data to buf

        ret = mpi->decode_put_packet(ctx, dec_in);
        if (MPP_OK != ret) {
            goto MPP_TEST_FAILED;
        }

        mpp_packet_deinit(dec_in);
        dec_in = NULL;
    }

    for (i = 0; i < MPI_DEC_LOOP_COUNT; i++) {
        ret = mpi->decode_get_frame(ctx, &dec_out);
        if (MPP_OK != ret) {
            goto MPP_TEST_FAILED;
        }

        if (dec_out) {
            // interface may output multiple frame at one time
            do {
                MppFrame next = mpp_frame_get_next(dec_out);

                // TODO: diaplay function called here

                mpp_frame_deinit(dec_out);
                dec_out = next;
            } while (dec_out);
        }
    }


    ret = mpi->flush(ctx);
    if (MPP_OK != ret) {
        mpp_err("mpi->flush failed\n");
        goto MPP_TEST_FAILED;
    }

    mpp_deinit(ctx);


    mpp_log("mpi_test encoder test start\n");

    // encoder demo
    ret = mpp_init(&ctx, &mpi, MPP_CTX_ENC, MPP_VIDEO_CodingAVC);
    if (MPP_OK != ret) {
        mpp_err("mpp_init failed\n");
        goto MPP_TEST_FAILED;
    }

    memset(&cfg, 0, sizeof(cfg));

    ret = mpi->config(ctx, cfg);
    if (MPP_OK != ret) {
        mpp_err("mpi->config failed\n");
        goto MPP_TEST_FAILED;
    }

    ret = mpi->control(ctx, cmd, param);
    if (MPP_OK != ret) {
        mpp_err("mpi->control failed\n");
        goto MPP_TEST_FAILED;
    }



    // interface with both input and output
    for (i = 0; i < MPI_ENC_LOOP_COUNT; i++) {
        mpp_frame_init(&enc_in);

        mpi->encode(ctx, enc_in, &enc_out);
        if (MPP_OK != ret) {
            goto MPP_TEST_FAILED;
        }

        if (enc_out) {
            mpp_packet_deinit(enc_out);
            enc_out = NULL;
        }

        mpp_frame_deinit(enc_in);
        enc_in = NULL;
    }

    // interface with input and output separated
    for (i = 0; i < MPI_ENC_LOOP_COUNT; i++) {
        mpp_frame_init(&enc_in);

        // TODO: read stream data to buf

        mpi->encode_put_frame(ctx, enc_in);
        if (MPP_OK != ret) {
            goto MPP_TEST_FAILED;
        }

        mpp_frame_deinit(enc_in);
        enc_in = NULL;
    }

    for (i = 0; i < MPI_ENC_LOOP_COUNT; i++) {
        mpi->encode_get_packet(ctx, &enc_out);
        if (MPP_OK != ret) {
            goto MPP_TEST_FAILED;
        }

        if (enc_out) {
            if (enc_out) {
                mpp_packet_deinit(enc_out);
                enc_out = NULL;
            }
        }
    }


    ret = mpi->flush(ctx);
    if (MPP_OK != ret) {
        mpp_err("mpi->flush failed\n");
        goto MPP_TEST_FAILED;
    }

    if (dec_in) {
        mpp_packet_deinit(dec_in);
        dec_in = NULL;
    }

    if (enc_in) {
        mpp_frame_deinit(enc_in);
        enc_in = NULL;
    }

    mpp_deinit(ctx);
    free(buf);

    mpp_log("mpi_test success\n");

    return 0;

MPP_TEST_FAILED:
    if (dec_in) {
        mpp_packet_deinit(dec_in);
        dec_in = NULL;
    }

    if (enc_in) {
        mpp_frame_deinit(enc_in);
        enc_in = NULL;
    }

    if (ctx)
        mpp_deinit(ctx);
    if (buf)
        free(buf);

    mpp_log("mpi_test failed\n");

    return -1;
}
