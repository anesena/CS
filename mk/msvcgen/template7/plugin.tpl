<?xml version="1.0" encoding = "Windows-1252"?>
<VisualStudioProject
	ProjectType="Visual C++"
	Version="7.00"
	Name="%project%"
	SccProjectName=""
	SccLocalPath="">
	<Platforms>
		<Platform
			Name="Win32"/>
	</Platforms>
	<Configurations>
		<Configuration
			Name="Release|Win32"
			OutputDirectory=".\csrelease\temp\%project%"
			IntermediateDirectory=".\csrelease\temp\%project%"
			ConfigurationType="2"
			UseOfMFC="0"
			ATLMinimizesCRunTimeLibraryUsage="FALSE">
			<Tool
				Name="VCCLCompilerTool"
				Optimization="4"
				GlobalOptimizations="TRUE"
				InlineFunctionExpansion="2"
				EnableIntrinsicFunctions="TRUE"
				FavorSizeOrSpeed="1"
				OmitFramePointers="TRUE"
				OptimizeForProcessor="1"
				AdditionalOptions="%cflags%"
				AdditionalIncludeDirectories="..\..,..\..\include\cssys\win32,..\..\include,..\..\libs,..\..\support,..\..\apps,..\..\plugins"
				PreprocessorDefinitions="NDEBUG;_WINDOWS;WIN32;WIN32_VOLATILE;__CRYSTAL_SPACE__"
				StringPooling="TRUE"
				RuntimeLibrary="2"
				EnableFunctionLevelLinking="TRUE"
				UsePrecompiledHeader="2"
				PrecompiledHeaderFile=".\csrelease\temp\%project%/%project%.pch"
				AssemblerListingLocation=".\csrelease\temp\%project%/"
				ObjectFile=".\csrelease\temp\%project%/"
				ProgramDataBaseFileName=".\csrelease\temp\%project%/"
				WarningLevel="4"
				SuppressStartupBanner="TRUE"
				CompileAs="0"/>
			<Tool
				Name="VCCustomBuildTool"/>
			<Tool
				Name="VCLinkerTool"
				IgnoreImportLibrary="TRUE"
				AdditionalOptions="/MACHINE:I386 %lflags%"
				AdditionalDependencies="ddraw.lib zlib.lib %libs%"
				OutputFile="csrelease\temp\%project%\%target%"
				Version="4.0"
				LinkIncremental="1"
				SuppressStartupBanner="TRUE"
				AdditionalLibraryDirectories="..\..\libs\cssys\win32\libs"
				IgnoreDefaultLibraryNames="LIBC,LIBCD,LIBCMT,LIBCMTD"
				ProgramDatabaseFile=".\csrelease\temp\%project%/%project%.pdb"
				SubSystem="2"
				OptimizeReferences="1"
				ImportLibrary=".\csrelease\temp\%project%/%project%.lib"/>
			<Tool
				Name="VCMIDLTool"
				PreprocessorDefinitions="NDEBUG"
				MkTypLibCompatible="TRUE"
				SuppressStartupBanner="TRUE"
				TargetEnvironment="1"
				TypeLibraryName=".\csrelease\temp\%project%/%project%.tlb"/>
			<Tool
				Name="VCPostBuildEventTool"
				CommandLine="echo Moving output to CS root.
copy $(TargetPath)  ..\..
echo Moving output to MSVC Release Bin.
copy $(TargetPath)  csrelease\bin
"/>
			<Tool
				Name="VCPreBuildEventTool"/>
			<Tool
				Name="VCPreLinkEventTool"/>
			<Tool
				Name="VCResourceCompilerTool"
				PreprocessorDefinitions="NDEBUG"
				Culture="1033"/>
			<Tool
				Name="VCWebServiceProxyGeneratorTool"/>
			<Tool
				Name="VCWebDeploymentTool"/>
		</Configuration>
		<Configuration
			Name="Debug|Win32"
			OutputDirectory=".\csdebug\temp\%project%"
			IntermediateDirectory=".\csdebug\temp\%project%"
			ConfigurationType="2"
			UseOfMFC="0"
			ATLMinimizesCRunTimeLibraryUsage="FALSE">
			<Tool
				Name="VCCLCompilerTool"
				Optimization="0"
				OptimizeForProcessor="1"
				AdditionalOptions="%cflags%"
				AdditionalIncludeDirectories="..\..\plugins,..\..,..\..\include\cssys\win32,..\..\include,..\..\libs,..\..\support,..\..\apps"
				PreprocessorDefinitions="_DEBUG;WIN32;_WINDOWS;WIN32_VOLATILE;__CRYSTAL_SPACE__;CS_DEBUG"
				RuntimeLibrary="3"
				PrecompiledHeaderFile=".\csdebug\temp\%project%/%project%.pch"
				AssemblerListingLocation=".\csdebug\temp\%project%/"
				ObjectFile=".\csdebug\temp\%project%/"
				ProgramDataBaseFileName=".\csdebug\temp\%project%/"
				WarningLevel="4"
				SuppressStartupBanner="TRUE"
				DebugInformationFormat="4"
				CompileAs="0"/>
			<Tool
				Name="VCCustomBuildTool"/>
			<Tool
				Name="VCLinkerTool"
				IgnoreImportLibrary="TRUE"
				AdditionalOptions="/MACHINE:I386 %libs%"
				AdditionalDependencies="%lflags%"
				OutputFile="csdebug\temp\%project%\%target%"
				Version="1.0"
				LinkIncremental="2"
				SuppressStartupBanner="TRUE"
				AdditionalLibraryDirectories="..\..\libs\cssys\win32\libs"
				IgnoreDefaultLibraryNames="LIBC,LIBCD,LIBCMTD"
				GenerateDebugInformation="TRUE"
				ProgramDatabaseFile=".\csdebug\temp\%project%/%project%.pdb"
				SubSystem="2"
				ImportLibrary=".\csdebug\temp\%project%/%project%.lib"/>
			<Tool
				Name="VCMIDLTool"
				PreprocessorDefinitions="_DEBUG"
				MkTypLibCompatible="TRUE"
				SuppressStartupBanner="TRUE"
				TargetEnvironment="1"
				TypeLibraryName=".\csdebug\temp\%project%/%project%.tlb"/>
			<Tool
				Name="VCPostBuildEventTool"
				CommandLine="echo Moving output to CS root.
copy $(TargetPath)  ..\..
echo Moving output to MSVC Debug Bin.
copy $(TargetPath)  csdebug\bin
"/>
			<Tool
				Name="VCPreBuildEventTool"/>
			<Tool
				Name="VCPreLinkEventTool"/>
			<Tool
				Name="VCResourceCompilerTool"
				PreprocessorDefinitions="_DEBUG"
				Culture="1033"/>
			<Tool
				Name="VCWebServiceProxyGeneratorTool"/>
			<Tool
				Name="VCWebDeploymentTool"/>
		</Configuration>
	</Configurations>
	<Files>
		%groups%
	</Files>
	<Globals>
	</Globals>
</VisualStudioProject>
