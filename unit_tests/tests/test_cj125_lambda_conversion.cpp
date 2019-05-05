/**
 * test_cj125_new.cpp
 * 
 *  Created on 23 January 2019
 * @author Matthew Kennedy, (c) 2019
 */

#include "efifeatures.h"
#include "gtest/gtest.h"

#include "LambdaConverter.h"

TEST(LambdaConversion42, Lean)
{
    EXPECT_NEAR(LambdaConverterLsu42::ConvertLambda(2.97322f - 1.5f, 17), 2.42f, 0.001f);
}

TEST(LambdaConversion42, Stoich)
{
    EXPECT_NEAR(LambdaConverterLsu42::ConvertLambda(0.0f, 17), 1.009f, 0.001f);
}

TEST(LambdaConversion42, Rich)
{
    EXPECT_NEAR(LambdaConverterLsu42::ConvertLambda(0.363516f - 1.5f, 17), 0.8f, 0.001f);
}

TEST(LambdaConversion49, Lean)
{
    EXPECT_NEAR(LambdaConverterLsu49::ConvertLambda(3.289f - 1.5f, 17), 3.413f, 0.001f);
}

TEST(LambdaConversion49, Stoich)
{
    EXPECT_NEAR(LambdaConverterLsu49::ConvertLambda(0.0f, 17), 1.003f, 0.001f);
}

TEST(LambdaConversion49, Rich)
{
    EXPECT_NEAR(LambdaConverterLsu49::ConvertLambda(0.814f - 1.5f, 17), 0.850f, 0.001f);
}
