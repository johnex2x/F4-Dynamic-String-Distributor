#pragma once

// Patches a known non-atomic Scaleform AddRef inlined into other F4SE plugins (CommonLibF4 RefCountImpl::AddRef
// does ++refCount instead of ++myRefCount). Each site is patched only when 24 bytes around it match exactly;
// a plugin that is not loaded, or another version of it, is left alone.
namespace MenuRefCountFix
{
	// Call at PostLoad, when every F4SE plugin is loaded and nothing renders yet
	void Apply();
}
