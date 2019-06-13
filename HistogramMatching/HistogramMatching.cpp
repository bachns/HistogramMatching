/********************************************************************************
*   Copyright (C) 2018 by Bach Nguyen Sy                                       *
*   Unauthorized copying of this file via any medium is strictly prohibited    *
*                                                                              *
*   <bachns.dev@gmail.com>                                                     *
*   https://bachns.wordpress.com                                               *
*   https://www.facebook.com/bachns.dev                                        *
*                                                                              *
********************************************************************************/

/**
* File name:    HistogramMatching/HistogramMatching.cpp
* Date created: Thursday, Jun 13, 2019
* Written by Bach Nguyen Sy
*/

#include "pch.h"
#include <iostream>

void image2Histogram(const cv::Mat& image, float histogram[]);
void histogram2CumulativeHistogram(float histogram[],
	float cumulativeHistogram[]);

void histogramMatchingChannel(const cv::Mat& inputChannel,
	const cv::Mat& desiredChannel, cv::Mat& outputChannel);

bool histogramMatching(const cv::Mat& inputImage,
	const cv::Mat& desiredImage, cv::Mat& outputImage);

int main(int argc, char* argv[])
{
	namespace po = boost::program_options;
	po::options_description desc(
		"The Histogram Matching Program written by Bach Nguyen\nAllowed options");
	desc.add_options()
		("help,h", "Show help")
		("input,i", po::value<std::string>(), "Set input image")
		("reference,r", po::value<std::string>(), "Set reference image")
		("output,o", po::value<std::string>(), "Set output image");
	po::variables_map vm;

	std::string input, reference, output;
	try
	{
		store(parse_command_line(argc, argv, desc), vm);
		notify(vm);
		if (vm.count("help"))
		{
			std::cout << desc << std::endl;
			return EXIT_SUCCESS;
		}

		if (!vm.count("input"))
		{
			throw po::error("The option '--input' is required but missing");
		}
		if (!vm.count("reference"))
		{
			throw po::error("The option '--reference' is required but missing");
		}
		if (!vm.count("output"))
		{
			throw po::error("The option '--output' is required but missing");
		}

		input = vm["input"].as<std::string>();
		reference = vm["reference"].as<std::string>();
		output = vm["output"].as<std::string>();
	}
	catch (po::error& e)
	{
		std::cerr << "Error: " << e.what() << std::endl << std::endl;
		std::cerr << desc << std::endl;
		return EXIT_FAILURE;
	}

	cv::Mat inputImage = cv::imread(input);
	cv::Mat referenceImage = cv::imread(reference);
	cv::Mat outputImage;
	bool result = histogramMatching(inputImage, referenceImage, outputImage);
	if (result && cv::imwrite(output, outputImage))
	{
		std::cout << "Match succeeded";
	}
	else
	{
		std::cerr << "Match failed";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


void image2Histogram(const cv::Mat& image, float histogram[])
{
	int size = image.rows * image.cols;
	for (int i = 0; i < LEVEL; i++)
	{
		histogram[i] = 0;
	}

	for (int y = 0; y < image.rows; y++)
	{
		for (int x = 0; x < image.cols; x++)
		{
			histogram[(int)image.at<uchar>(y, x)]++;
		}
	}

	for (int i = 0; i < LEVEL; i++)
	{
		histogram[i] = histogram[i] / size;
	}
}

void histogram2CumulativeHistogram(float histogram[], float cumulativeHistogram[])
{
	cumulativeHistogram[0] = histogram[0];
	for (int i = 1; i < LEVEL; i++)
	{
		cumulativeHistogram[i] = histogram[i] + cumulativeHistogram[i - 1];
	}
}

void histogramMatchingChannel(const cv::Mat& inputChannel,
	const cv::Mat& desiredChannel, cv::Mat& outputChannel)
{
	if (inputChannel.channels() != 1 || desiredChannel.channels() != 1) {
		std::cerr << "HistogramMatching Function Error. "
			<< "The Input Image or Desired Image does not have only one channel"
			<< std::endl;

		std::cerr << "Input Image Channels: "
			<< inputChannel.channels() << std::endl;

		std::cerr << "Desired Image Channels: "
			<< desiredChannel.channels() << std::endl;
		return;
	}

	float inputHistogram[LEVEL], inputHistogramCumulative[LEVEL];
	image2Histogram(inputChannel, inputHistogram);
	histogram2CumulativeHistogram(inputHistogram, inputHistogramCumulative);

	float desiredHistogram[LEVEL], desiredHistogramCumulative[LEVEL];
	image2Histogram(desiredChannel, desiredHistogram);
	histogram2CumulativeHistogram(desiredHistogram, desiredHistogramCumulative);

	float outputHistogram[LEVEL];
	for (int i = 0; i < LEVEL; i++)
	{
		int j = 0;
		do {
			outputHistogram[i] = (float)j;
			j++;
		} while (inputHistogramCumulative[i] > desiredHistogramCumulative[j]);
	}

	outputChannel = inputChannel.clone();
	for (int y = 0; y < inputChannel.rows; y++)
	{
		for (int x = 0; x < inputChannel.cols; x++)
		{
			outputChannel.at<uchar>(y, x) =
				(int)(outputHistogram[inputChannel.at<uchar>(y, x)]);
		}
	}
}

bool histogramMatching(const cv::Mat& inputImage,
	const cv::Mat& referenceImage, cv::Mat& outputImage)
{
	std::vector<cv::Mat> inputChannels, referenceChannels, outputChannels;
	cv::split(inputImage, inputChannels);
	cv::split(referenceImage, referenceChannels);
	if (inputChannels.size() != referenceChannels.size())
	{
		std::cerr << "Channel of input image isn't the same reference image" << std::endl;
		return false;
	}

	for (int c = 0; c < inputChannels.size(); c++)
	{
		cv::Mat output;
		histogramMatchingChannel(inputChannels[c], referenceChannels[c], output);
		outputChannels.push_back(output);
	}
	cv::merge(outputChannels, outputImage);
	return true;
}