.model flat, C 

.data
 __imp__EncodePointer@4 dd dummy
 __imp__DecodePointer@4 dd dummy
 
 EXTERNDEF __imp__EncodePointer@4 : DWORD
 EXTERNDEF __imp__DecodePointer@4 : DWORD

.code
 dummy proc
 mov eax, [esp+4]
 ret 4
 dummy endp 

end
 