/*
 * Copyright (c) 2023, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <imgui.h>
#include "imgui_icon.h"

namespace ImGuiH {
ImFont* g_iconicFont = nullptr;
}

// Forward declaration
const char* getOpenIconicFontCompressedBase85TTF();


void ImGuiH::addIconicFont(float fontSize)
{
  if(g_iconicFont == nullptr)
  {
    ImFontConfig          fontConfig;
    static uint16_t const range[]    = {0xE000, 0xE0DF, 0};
    char const*           glyphsData = getOpenIconicFontCompressedBase85TTF();
    g_iconicFont =
        ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(glyphsData, fontSize, &fontConfig, (const ImWchar*)range);
  }
}

ImFont* ImGuiH::getIconicFont()
{
  return g_iconicFont;
}

// clang-format off
const char* getOpenIconicFontCompressedBase85TTF()
{
    // TTF font data for OpenIconic font TTF

    // SIL OPEN FONT LICENSE Version 1.1
    // https://github.com/iconic/open-iconic/blob/master/FONT-LICENSE

    // The MIT License(MIT)
    // https://github.com/iconic/open-iconic/blob/master/ICON-LICENSE
    // 
    // Copyright(c) 2014 Waybury
    //
    // Permission is hereby granted, free of charge, to any person obtaining a copy
    // of this softwareand associated documentation files(the "Software"), to deal
    // in the Software without restriction, including without limitation the rights
    // to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
    // copies of the Software, and to permit persons to whom the Software is
    // furnished to do so, subject to the following conditions :
    //
    // The above copyright noticeand this permission notice shall be included in
    // all copies or substantial portions of the Software.
    //
    // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    // IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    // FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
    // AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    // LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    // OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    // THE SOFTWARE.

    // File: 'open-iconic.ttf' (28028 bytes)
    // Exported using binary_to_compressed_c.cpp
    static const char openIconic_compressed_data_base85[25385 + 1] =
        "7])#######cl1xJ'/###W),##2(V$#Q6>##u@;*>#ovW#&6)=-'OE/1gZn426mL&=1->>#JqEn/aNV=B28=<m_)m<-EZlS._0XGH`$vhL0IbjN=pb:&$jg0Fr9eV5oQQ^RE6WaE%J%/G"
        "BgO2(UC72LFAuY%,5LsCDEw5[7)1G`8@Ei*b';9Co/(F9LFS7hr1dD4Eo3R/f@h>#/;pu&V3dW.;h^`IwBAA+k%HL2WF')No6Q<BUs$&94b(8_mQf^c0iR/GMxKghV8C5M9ANDF;),eP"
        "s=L%Mv6QwLYcv/GiF#>pfe->#G:%&#P$;-GItJ1kmO31#r$pBABLTsL:+:%/-p5m6#aJ_&FZ;#jL-gF%N55YueXZV$l`+W%G'7*.p+AGM%rs.Lx0KfLwF6##_OB`N1^*bNYU$##.,<6C"
        "pYw:#[MW;-$_C2:01XMCH]/F%@oj>-Wmf*%%q8.$F`7@'.B.;6_Du'&hF;;$63Mk+MS:;$l9r&lW+>>#YvEuu=m*.-H####Y4+GMQrus-Su[fLK=6##'DbA#bFw8'tB6##Lhdk%Nb#d)"
        "-l68%<a60%*8YY#ksnO(meGH3D@dOgq#Sk#<@KZ+^2bY#t=58.Z42G`R5pP'R#k8&1p^w0%2tM(TDl-$nq@8%uP&<6FMsFrkg%;Qk:MEd]'CJ#q4cQumLuFroPu>C&8FB-#@'$.8mwiL"
        "KO2EOR.tB-1N5</<PQ?SQlRfL^5fQMK4X$#8e`##9Q@k(xsFA#cFGH3*/rv-@0steAL@&Mt98'RE:6G`(q1@doX]'R>$hiPchu,MLY5;-]Xo34camA#@YqlLLxQ2MIMh1MB2tuLjV;;$"
        "PE)B4-Mc##=(V$#3JRN'GRes$-M:;$eN;hLGSrhL)kL)Nv$Q;-h8Ee#OYLxL/qPW-+l*hcw3Tt:K=/g)&QD.$s@o6<40)V;=6axT1F<U;2OWq;.]BxTR2###;:U;-<3]V$]V@^FJR@%#"
        "8l#U7uUO.)Y9kT%1YUV$Yp()3GU1E4m3[5/sgw,*wqq8.5&7C#)#xP'hT%;Q<a&buYwaVaJjAZ#/7:*j>[x@A1P&,;nl8IW*Yn36=b/(G*8FW.KkqrUgt]_uR0.g>Z>,%&$),##50ED#"
        ";EF&#8EaP]UkIJ1P*nO(@nBD3gp/Ys%*c,/Wp`IhM]^'/B67gL0mM<%9@RS%EE.<$(iNn0&DJ1(*O13(0&Ws%C8I6oF'XMon(BW#Ej5/1#iK#$V@pgL5W=huZXAwR'qAZPH(<?#%G3]-"
        "R4B>QW<Q]uIvg_-5Or`S*J);;RmwiL*BauPcOB_-66lKYg,3^#r+TV-jO@mk5ImO($FKx9R:xIQTCfd4XcE,#8EaP]VkIJ1W9m9MqiRdF#jaIhUm9DC)Pb3Fq';dMJ:>GMU'MHME]sFr"
        "$#bIh$ErFrtE)Xu*S.GMsFZY#xx'&(-hBD3O4g]uc_.08/tf2`_^7I-PK7I-dn6]-TYwTM9en9M3*-&4%lQDO]H1[MO@YoPM8,,MU#<?#0q3L#r&_B#Gk5`-+XV$^wAI&#MgwGMe/=fM"
        "%2rAMwM,i87d0^#(tAh8IG1B#8E_#$uLbA#=2vg)<j8>CoeY9C3[-CuBu[*$#xYr#2w@P#3(CJ#G*2GMlH^v-KV;;?BT_c)lH*20%'%<&DI$,*DiY##=Z[0)jiWI)qV6C#2-#Q/8bCE4"
        "d.fZ$aSIWABTgk0;-LD?7L1a#d:+d4.:L^u@Ft%,O27>>$/Ib%@s@8:JrksLPQuP=Wj&2XY'f*.W>%H?Jo*50D@N'6@:&&6Mj;5:[G>',%/5##sIkA#?V)2/9(V$#rA+KWNVB.*0Zc8/"
        "_YWF3p;:R/g;2.I07b2CAZAb4hLY['IR(<)[8UCCx5B58Xatm'Dn35&+^L-M&J_P8H79v-P>/B#9]SfLYWY95F5kWo[iJdo7PFa[Nk9:<bmp2<`1g2$^kYhLMbQb[P)^F#&fAguE8*C&"
        ".'wwB_B(7#1Yu##;PF$/GRes$c^HwBH$nO(gv8s7EPdO(jW4G`Lq#Gr3RgIht28G`]n&=(Vu_?###b9#`KGf%;T<A+&Zu(3%'%<&_T/a+bhV?#+crB#;0l]#P#G:.rkh8.mNv)4kDsI3"
        "5;`:%q2Qv$lf/+*u?lD#g48`&/0*>P^-R`E7+R<C0F91p*AM9LpLL=Q3H?.q]PMP0*+_HM3bQ;-`78JL$R]duA'OI)ZFX:CxH#OKvCnf)Gd(APJCZY#)J/-2IdS2^M4v##;Z[0)?sGA#"
        "vx:g)Z4jd%lUFb3(ctM(7'$,&H$^>u=NxH#6J8I&tS?.&(w?w%M&CJ#nxkT9HoMs%Ri,#:fs0c7fGA_#kSuIUZ42G`#?#,2'ti21;ix+MG3gF4`cJ,3)_Aj0cI9F*jpB:%cn4[$UJ,G4"
        "[uUv-?]d8/hJL:%+SN/2X,n.)1Yl>#4a&m%;Bjo*G6'j0DTnr6>vl[P^Su>#D5UlSjXT)5er^)%7R2=-kL)APv<qa4<IK60tY#c4<q@?PW;e+WLuuEm05Vm&fK(,)nM5N(f%JfL`Lu(3"
        "9UsNOj/@tLh]8f3.m(u$Uxor6]]DC8ukm/N3vOG=Mo&vHc2Ii4iJ<cFuP@+`E'flLbcK`$v)$$$PrC$#e28]%/5:$)V'+X$)lNn0vpf8%B:Y)4ZE>V/isSi)XSf_4Kj+5A3/mCW-nm<S"
        "`Tr6C?9%0$lKTATZ(N=(=BPEd++wH?Ukx/1U'+&#`,Gj^8%L+**<m`-0]kHZ(LOA#vkh8.7/TF4o=]:/Z(OF3kl-F%3F6eMJGsFrY3Kt7lAkHuNBAJ.3dT3C7Ot2CSeKD?]a;*eB[f%-"
        "6K^'RB,aB#FP*2MSiaIhwq$vJZTEV&biqr$07>>#8EQJ(6uv9.nXAouu[XO%s[?>#p?_&#aJxjt^;>_/^1[d+=7B_4s7En3+I@X-vu/+*X0fX-]p*P(&,]]4qio05A@N@,.>T@,(r?5/"
        "=KeA?u>kB,pSaIhUT%/2.dOCjA/#2C7CA5//`Nw$*jnA#abDE-)aDE-/MXZ/8N9R//G/JdTa+W-@Isp^Z_?##EdS2^4;5##af[^$/6fX-m'4]-7^9Z-HLQ;-.`6/1xq7t%uR$w#tPR6D"
        "CIm,&=G4?Is8K%#fd0'#$sb&,`X2$#p%c00gEW@,QFVC,732m$ic``3t<7f3I.=h>K7)qiFe8V8CwBB#'8:$6-^J6atvN@,P;I80eF&b*l-AU+Q1(`.JLR$&cIqH1Vec<066V9CIBkp%"
        ",<$'85fK>?uxwV.fE4g)Een2B,R(f)o77<.hp?d)BiaF3iEo8%Z_G)4@O'>.g:S_#OJ,G4oHl2Eq<@12u)Yx#IM6u?b%]=YnAv6Jex/U%ZRQ;-P2>.3hC7),'BW<BvIo#';9l<'$<=8A"
        "QI%##SJ%?R#H0%#hj9'#G`gT7LoqZ-$QdR&*oNn0@hmV-+NTj`qc=00vc%H)+,Bf3rQim$VKi9/(o?X-^hDv#@81G`Ig2Au0aYK)uXAA,/Pg;M+q//1gFB,j<34G`R=wP/ji1G`]d`oA"
        "Q$1i#`M66CL=6##)CQ$$^0+&#Kqn%#7mJR8@R6C#a?T:%cKff16t'E#E%D'S1lh8.VntD#$be#5@=e/1#6RCsVJ$F<tI`?#Of6'5r0Er:3MBk;spA/1cbg@OmWuI<^'s?#FWS5<3QP-."
        "DM_*597on2$&###ZYw4fITr+De1'<3S=>6sG)'J3W#[]4<PsD#%f%s$PL?lL14(f)MCr?#sX.r8aa#K)Nl/BHERX/2ucJ:/mu[:/vBo8%r:#?,h6n5/s39tU3*Y<UZ?85/Q4vr-VSi/:"
        "#b<c4-[xo%ls>n&;[qu$0FLS:/.r%,&@ed)$QlvG@5uj)Oi3+3G0Z**uJMk'^@3B6.xl$,eAru51Qkp'&.-,*dLW)3.,RW%EUu^$sZG0(pS4=-JvW@,8PQc4RT8,*IPgf)3;N#5G&MP0"
        "U`2p/Qkf34,N_2:o76HN7.7W$]YO@,_NG/(,a'W$%'N50ohYgL_IGG,<;:v-2@`[,=bT]$?/5##'AlY#ID(7#Fww%#QYmq%WqRJ1`tC.33Tlk0[IBN(3pm;%`ni?#A=M8.cc``3eN:;?"
        "AWw8@k%#s-00J,;4YIQ0&Z?X('P7G`C)j-&JPciuqvC)*#BU3(UH5/(omLgL0G5gLMQ(]$Y$3$#E@%%#Uqn%#fKb&#Rp8X-@[v(+vuRL(QknW$-xNn0WTP$KC[/Q/:%xC#)vsI33E0N("
        "H1:)N]*WZ#1H(0aVHt1':Z/GM%7Q;-%#h3&F'Bq%n1Goub&]:.CCK?d$SF&#T=P#$b,`tL)V[*._1x+MPr@X-Uokl8JJK/)$8h9MO4G^#P^.3M>7(#&S:A2CmEx>dOI5r%:^iqM&_V;:"
        "i51g*lBel/a)12'oon60Jn))+`'fJ1`tC.3?l0x5oSEU-a1M9.F)E`#dDp;-JY#<-GOH&%H0B^^<5=E$`Ge,Mu%2&7&bsZ5oYAA,?J<MKYsslWoV[(6DE;&$b/<-&hpE$M$oO07*pcW-"
        "j:?NMCQFv-(oHhLL2JG=$,>>#B$k>dKV;;?/addFXh)v#`._f#q%KoMhC?>#[CNJMf:-##-Q-E0rB%;QaVS;-#'e32tYg59ZF;s%BOP]uA_$'Q)uCt/OKkA#Bua1uiRJF-[d'-%q%5kO"
        "vfP&#(K6(#=3CR0*_S[#:H%O(xNUA$6*YA#Fr/l'V::8._%NT/-K?C#ued;-1rru$JMQF%'1u.)M[4;dv<[5/H5_T%:='d)/OTF&*j`B4')?N#]_h[,>eq/3l;pNM1'A5/rM?R&*dgAt"
        "UZg6&7NF*+?jTx=EIIW-@NSKug0pTV.52G`4CNP&?&3-&?A/'H2Bx9.^1tM(b,sFrjv'Q/'ZedubwtBC/:CB#jI:0C6CBh5E8I/(dcT5',m?T%eee@#.N?C#qw6u68AB-QI`_M:lpZEP"
        "sTK^P9?%RPLu6j:kmQEPKF^w'?7%-3KV;;?J(WqgNt#`40<t4SYG,juMZfF-)dGW-^Wk2DNLZY#Kp+)O8pID*IF)?#h:VX1@93/M]VXKM_oX02SkIJ1lF;?#?W$9S$tn=GE$M#$Q#HpL"
        "<D%/O(n)U/[[2Q/8a&;Q%a3^#RWU@-nR.K9u?:@IY3E$#;P6*<TH+O4hKr0aWl;;?xpSBZ,K<@I_3EiM(IkB-/RkB-,J'O.kE$`4(9a;%a0x8.>+nDS2BI4=E&=h>]^'Q/Z9V;-YOho."
        "<TqA,B/^q'%vGA#gEW@,YG8f3vf%&4O$/i)cn=X(eOnH+s1gF4`XdB,1>MZu]8+-;]kFF#'''k0[gX7IMhIw#Ri3+3Og<X(+gpk9QrD7*416AO7CsEI@cR30R,Gj^]9kT%d`>qpfnQQ%"
        "e/0i)(@n5/6a=a;KfZ_#4p#g[oEkCWnP%3:)puH;f3Yn*q=jg.:KVK2HR+8&5R&/1t.b8/mRXR-Oh#L/lYWI)*^iF=t%'Z--nii/S;3,)s<RF4#pMB,1H4Q/m$UfL_qtFb-:jB5l.$vJ"
        "2uY<,7;IwPvMc>#^XL@,709JSBlvuQ#sj?>drN+3Pr3&7b&WF=b'4fur>gf)?N=v-=Kjr7wv#K)LVsQNXSlwPPaE<%%H>[01lXI)YAOZ6#FPcMnGkIdt'e'Jh)MUdPco=d`X@>d_d?]/"
        "NX.i)Oi3+3AS.n/DEaP]ZkIJ1Bd]N/:2Cv-6I7lLg%J#/Lcp77DsX<h3KKwMOw^A?,C1na;s4T&x6v^]$@O9%(iNn0L#>u-=Fn8%$t)Z$<QJ&OvDrkJbb>a03c7uQOP]'RsxS@B+w<2C"
        "$eXw,3d]'R]'^F-LvKl3WU'^#87>##;(V$#QJZ6&0;5##@%Yw2(k^#$BUW:.eAgI*074D#;9ovektS9Ce*loL*uLp.2w@P#gO:1#/s#t-AQWjLA1(?-G-ml$c#[]4bQXA#`&ID*bQs?#"
        "(LA8%]CAC#kHSP/GI0P:?fHC#4(=(,wB3r&PC:*31Nw+33Ij,WxMvuJ3)WVRh5l?-mO4Z-Nv4wgwMPT.,GY##TFk3.je'dMT,AX-<<xX-c#g*%k*f%u_I,;Q'rw^$`Pvf;E9Z.6aq#.6"
        "d*6.6Xax9.wF5s-6/XjLdX2DdG*B_8%oK^#_.kFdx-Ns->0niLBpID>e9+@nf*,s/aNP;dextBCY?:@-[x%d=<x&#_4:ww^.oQ<_G.4[^n,?2CDOUEn's[j0Kb7G`%K>G2DLKV6dl5D<"
        "-W<T'5ek#7T#=i26k/%#>Q@k(4tFA#K;gF4C'+W-`+[^Z2UkD#A.xJ1u3YD#o]d8/@jQv$0/D_&(C=m/H,Bf3Pv@+4*ZKW$RbZV-:'ihLO`6<.`6?8C&)$?\?Yo-x6KZn^43YAA,4&huG"
        "]g&i)#VWECnp@A,Be//1?`wA,4lW0R8MnA-kvjS8ln3B,mbX9CkF:@-bI`t-[I?M93+s20jbP;-3xqr$4FG>#l7Fb3js&i)I>Cv-s$gM'OcO4]O^>r%3^lSdq_adth<e##E@%%#6`gT7"
        "l1oh(KF.<$^&ACmmE<R%v-sV$95<+MKDlh':E1qLZMCsLb7cB,H]0hL6w//1g0Vk+]gK6/7rC$#0KAN93TkM(GPS8/nr@B=5bOAu&J[n[d2&Z$tCF&#b(p#$5FNP&IeJH*)?[s&N:+A["
        "HdF)</.'Z-^I-/:#@-$$xn>V#fnG688MFAu.Zo?NosS9C+Wq-$,kq0Y5rJ&vJao,;#(C7'I]>c*ed^6&Y'fJ1`=K,3cX.f$^#lP9TAu`4/I#H28)qj&s8Kf'clnP'IEPR&-<m`-LH.wI"
        "mIAou)ouN'uK,:)0h#h1JL7%#R,Gj^x44.)_xefLa#1i)D>`:%J#G:.]L3]-kDsI3d,ID*dkS9C(8_33L#v20_gB_u(%x)$>m;U)Y12mLS2sFr,bdt(LF.[B_Tr22kHe##IXI%#.^Q(#"
        "Z=sd2PC:J)KF.<$U^nVoVB:=%iZeF4uP2)3i%7V87:Y)4/D-HN<&iM'Fvnb0O)cB,4&Jw#R>iO'Q'4$6Ga7#%-DDJ:j'VhLirMP0v$C>$lD.gL*kC80AYlN9qjTn/3`($#Fg99%OT(]$"
        "K1>u%$CZV-1q//13RgIh(17I$E/<v>8QX]u.d*X8AI]v$d>f;%XtRb%]:75)Vi-]-(^&@93_V;-YG^e6J7ws-f$L-MDMYGM_0SX-(=.p.n5UQ/D/)GuI/15AV?96/3BZ=Yxo[F:HQXLs"
        "Yw2$#/h>%.HvC;?'*Zg)n.rv-dh.T%ve#`4lYWI)QNv)4%ZFW#o(p^[2I(I#-l*#7Y<2.Is;#eoQOuf[c%q=?;5T[8`6gM=>-Q2iiO2G`K[VV$7Rjl&FnfL))?[s&bvY3'CI[s$Y-oJ1"
        "E=R<_2pDs%*$[g)mBmGMN?H)47p&;Q;GB0uwRE2CsIkA#NKX_$>aKs-8(P6MjL1_%D&###aMwFrsqou5P']m,'fNn0]_OF3JIV@,M$g://]v.4i'mG*e4B>,j^Aj0<kY)4ax;9/Tn.i)"
        "RJ))3wc``3onXj1:s?^4_Bq:d$PJ@-xq=K(NljE*b<bc2(3D1;8uO]uFf<C4](e3*4`[?AuS-*3;1(p78]F.3C;TBR6%>H&Ul68%=jQK%r$'et1-;hLRZ/@#$#bIhP$+W-U9:wpx215M"
        "t'918/Tbk4tJ*B.^,QJ(29'.$Xd`#$XoI08&s9B#Lu@P#h(>(.p9_hLxT?R&=.:%/bfQv$&tc05&1=]-6C_B#6G<GdT4Nm#Dvu8.cYxi=+'a216a.c%f#ak=dbI<$RnsK)_;W.3^J)B#"
        "=;`C&I0@)KG(@$g]s9wgaGA*n2Lh'#%&>uulY@)$U:r$#7Yvh-t'YKj*$&],+3j'%j7o]4apCg%q]WF3;X^:/oNv)4QdPw-L%O;9`H[n[ww:v/,FeB,tKO@,ep3$6vqf5'nBI(R>Nuf["
        "4:#mFxCEZuG5EC=S*cB,vj%.MMJ$291i9^#q1:kFNMY>#=5^s%2H`hLf%f^$2om8.]^C8#Qnh9VLVUV$i8SC#(<0/1$J@;?p/W;-jWGouR7Fv/Pb./1<)sFr^n-q$NsLB:K(Hg)$%nhM"
        "0U.Pd/A:.$b)g;-w#Sv&R+no%:x,W%6oiu$/%@p./6tM(.@wq$tTQ^#+r0Pds@O]u@#pG&i435&<U35&Q/bT%-Duu#,.aV$6<Xq.aXa>.>c5g)NEFs-_gPS@:;xp&FI2Pd=1.AMUV7>S"
        "un$W`w5V9CQ7T&#h-7m%qnrr$>i7@%PBBN*#DRv$t0;W-wod;%m^D.3Coh8._V_Z->-w)4D.7x67gurBK[3Ib(/&,;MV0?)<-OBkvmYf>e_VX^m'1,)0D>;+1H:q#`63.5($(U1roT3<"
        "IA]C@PGd2CKoxEI.*nMjpg&%#6x5U7n+oh(qe)<-]VsR'e)ahLKZP8.$ed19p_v;-Bviu$vj4N9G'Z'?>IKC-#_@S/j;1g:?@(XS<U?a$K5>_?tor#($o@g:1gF>d_[i5r_A-$$LYu##"
        "FdS2^Crl##9H%O(J%1N(m`0i)x1tM(Zk_a4U&>;?`04]<<Lt2CP#=2C*S+RNPjo@S(9a=#95>%&g&p%#&dZ(#:efX.mqD$#*crB#AY>v,YLU8.Od5,2]AtJ:cT3T@D&Du$@]WF3cb(f)"
        "0Zc8/xS#8RJ40:95nkp%;Omb>igZKN>P9L(44'k1K_K6MYsslW)aog(:_i[,p)IG=X13G`O39Z70$Qn&D.4x71Y=)-G@-u6KU4x-aSDe$)D([,.rbi(jEelL+iZY#kSuIU^CMc`j*io."
        ".Y=W.9offL&(tu5kiE.3[Z*G4AMbI)#R6.Mp?Uv-&cUNtQ/TP/>;gF4See@#v2=v#RD.l'ncgQK=SHt7prgf)3lW`uE=AX#TVGe)B[0DLu33G`))[#8xf8qJSKOW',rKJ)=CEL#FRGE7"
        "tql?,Du5]MwY1f+TRQ##XmWF.4MG##Z.n#nW(hh&fK8<-]Cex-Ap/kLfQAa+pNq12nS.d)a5eD*jE7f3EsmC#2RQv$KVM_&:OQ^+IL>n0/[?U&FF76/5vai26bZIMaWPv.5Kuw'#h^>$"
        "jP$ND.cTs9FnNp1ZN%q/7kgv7[RiXu>['jC<;v<-OH(5'YsrQN,GkWSF&oO(:76g).CwX-DrS9CYI[i.51Gou46E/%<gOSc'cWj&%ZI*-PX=F3>GVD3<Qwb4-Pu>Cnn(ju%'DC8oA5>d"
        "Kn6mA^I6Z$(rQ-HmA4K3,0fX-H_$gL*]d8/sRh`.N-v7$hXWY$+w<2C)+r772=hf)qx8;-UsgY>FPaJ2tKZt.'fNn0=R(f)YsHd)7>1p$V>%&4&=(u(7_Aj0Y$n;'&QVA.OGg+4GF[?\?"
        "]g8P133'#*)LwLM@/2&74jj6/id1*3URQ;-8khh;$p8?,%>2e;pFNPM&+*#7;Lo0:X(Z<-8,pL:8PQv$2+:wg%&>s6t++E*$C;W-NZAX-ia&;QK=S?SF5nNSkko@SbCNJMYI(q7.uR_8"
        "`4`v#(iNn0)+niM^uQ2MgN&;Qi2_$9,lO]uKmWnLQE%q&=M.>>6oA9=AZ$B=`82t.$5K+*X:Ls-4h&K1`1[s$q[2Q/$8^F*rHNh#S5WF3NP,G4hO=j1HZU&$ZktD#5>PjL=`n]4pqi?#"
        "H&,J*/]&E#AIwZ&#s<h<PaM4'*4$O(FEPN'm4#(+:LL;&E9gq%/i1E4>E(1;>'SD*kZdh2`Dxu,mpP3',XTE#8O*t$5`tFb2_Nv#nI3p%YPw)*k9ob%$XA=1ruVs%a7kI)@=.<$kGiE#"
        "N4GD4RSE**[haR8;0h:%71#m'N1gT&f;m7&Ze7W$TmLg(ZHj@&W+2?#Mp:4'M9=x#%Sdi&ZFNP&DTo&Cft9B#jPD<%TbsJ1'<1^#_<m=Ga:)I#h_/rB.xCT.dC)I#Ulu8.3.`$#v_B^#"
        ";r_':cKQYS,v:T/(N#,28YR]4He,87UH^#.f1))5EK[L2%f8q/Z)l>-:CHc*qsJ<-hHFg$?:fpg2-RAOi-5.Vx^WR%.eY9C%mD('cFM<-;^TV-<KtglJFlW?62vG*&)t.dDjZ-d<pW,M"
        "jcNK:SK4Au9^)t-=1.AM2k^KOVj*J-hVOJ-S@jM'''lA#&K-Z$3.5F%F9OW-n=.`&i<F]uSwY<-Ot/gL3S$m8N(:7'^CPV-XK+n,gBFt$)lNn0k?8m85dTN(]SWF3]+:]?q0PCHuG^[#"
        "Tvgf)QQR@,ZD;;?g$a?u3*&v5P]sD3U%-w)tWS@#@nh(*iip[krQSQ,VHf;-Dwd]/k(@D3LXDT+['uJ1.;Rv$BxF)4TMrB#`i.@'3)4I)5Djc)4gE.3jLBk$dGG)4[V)60B>'i)56tB,"
        "M(w'&xOO]uW?DX-2'B(&-C$_#wkjf)-pQ@,2W>W-u/KF%3.(B#j_n.M8=kf-XLL-Q%]L-QbiT-Q8n@.*u^Op.jAqB#aU%LGDF,gL>fxd;]Fl)4fc5bsDdQ]/$'Cn)'<)iHRp)IHIgdsL"
        "T;I]MOjNg=aupbu,6^gL5D6wJ>%r;-Jr+G-Lq;Y0`qIJ1tveF4Y0%.MiMC:%HeolWDgC5/^%AluJ<cwLI[qI=j@[#6$,vx=Fh6gLY&_mA2VLv%*<x9._HF:.>T&;Q7CNEdqDDe#b?^6$"
        "w71Pd(l*3ia0E$#JepeF9HAX-*V3D#?Kaa4o4P;-uYZ9Cj)?2COqf5/ZQdduCLk%Mew%bO>x2$#u'B?.CfP##$#d;-=5ZX$[aoI*]kbF3]%p-)F53.)62m'=WlK#$U=#-ML(i%MOA#I#"
        "nPj#$A4f:d&?-9.;M7%#o&U'#,]ZZ-^L)$#<Z[0)lw6u6J]B.*-<Tv-*Lj?#f1ie3CmQv$^^D.3k=8C#tBo8%gYiM1'cl>du@cI3,o(NE#Po'&^)T1Mf;*i29Q4;7c'(E#ANp;-PwNn/"
        "MgAa5balSVO>.Q/P0_C6AWae36C(C&G/5###oKB#,DF&#'EEjL?YqU%Y]J3%;cLs-aSV)3ig'u$?S<+3FLRLMIU8f3w<7f3owsM0`0S(6]u&)GiLY<Bm@j;7c4.1MqeP;-*[9%6wP%a6"
        "3::e)[*%u67N[v.(q3HMikT]82^03(9W?K%Q93-v7*7g).$$E3m>sI3NY&E#Z%#Gr1qD)Wv^m).Hx&;%Nx<2CFm70C?\?1(q2:d*7#Xc2:Ti>g)6sLlMl0.q$e6AwY2^P[PAa;;?enPE>"
        "(cS,&01em8Q``'8(8F`S<3KV;0M<dteWj$#OdS2^B=qp7p-m<.3,Jd)]_G)4_V8f37'Hd2Frj2RMR7m'nOgIhWwZ>#S)oShe#s%gvs4/C:12,2>Bfq1%%).M%DYcMWg;;?[75;-CdU&,"
        ",h:_A2bAW-HXKZf?dnO(/,-J*AeL`$/DZ;$x,rFrXkuT:X^d4C`,LEdTCYG,qA6FS2=hf)qq[d#Gi<?8jPC*6Z[?##;(V$#PDQ6&5a_m/X36H3VF3]-`(`5/$tn=GUM6qL=LL?d,:-$$"
        "&.kFd>VAt9<NN)#'/###h=PxtIMMJ(.`JV6+522'PUX[7T=(/)F?r`$l]B.*1Wj;%I/X'%SSK5Bm;pY(uf]Y,LVZ9/eIfm0+LTfLvO5J*2u%T.jAqB#1Il805cVQ&q>6R&e^B<B0N&;Q"
        ":s,W-/Snx'@^s'+^I%s$sMA@,g7,_+Br$W$+'&gL6hM@,b.$$J@%uAB_m$%'N3c%'Z:U>,jvOA#b`vn&JCf*)R)cB,]7s^oQ>uu#eBF&#Od0'#oLXs50>cQ1=G5##=Z[0)okY)4:Z]F4"
        "(cK+*x_OcM&t'E#x.<9/8C)X-fpPA[Rp7jL&c6lL#0E.35MI[')J+gL2a+c43Fa$72r.DC*Pr+M>Up_4X(';S+RM@,FQTA#&$_f+Q6@iL^^Q-3cF@q%J(oQ-5W:V%sa;e33a_`5a<=DC"
        "tT3>dE-lV$8wMfW*+n@,OuH6S'8_],G(hKMb#<e3Z],@#.TW'8kT6%-3gc40udp(PAR&##%)###QwqjtEUai0*jTS%.E-x%)iNn0mf4K0K_`8.>;gF4`m]Ku[+'b?fo+G4X_giBJ[lS/"
        "ctMm'P[cQsYx6W-VU>qrSK;^#X]J$pm2>w&]3FX$jt$A,bs(k'.e3.NO6Q$,UG(.3AV)d*m0V@,kY[-).UwHN<CUf)-_`3t0<R($@uvE2,W(v#8=nS%-bi[,;uL;$``F'oS#Gd3=F=gL"
        "2%52(8UYgLL8,LN5`SP':,:W-6wpgucHh@.Jn@X-sBj;-9a^s$J)Z20$#bIhFH*;Qu5p'&FKEx'00O$MGoU3C37PF%MSl##./Sj0Z42G`5FNP&M,@D*fh18.(N#,252Tm(u4eW.D[Zc*"
        "j,Qn&9SG##@Q@k(#dCp.pN7g)Mkgb&T)cB,@qa>d$jV=YDmZW-?kWX1RLg+uKI%>Qx$EM-+61d/IIvu#3wFQ#K%9g1oqP;-K#adun,f%ut1c98B[X&#-,K9iW*+T.ElY##>]&(10ImO("
        "ij@+4auRv$#@F,4Mk5.I=Jr@Cn=#AdU_iM=M&CJ#':Po(P1T9ChDUO&9&:Xo)sLh$+6)Z-hfDu$kDjB#'1Lc5wo&vHYA,C8v4(_5-+7f=-YV(ZW4a?>^qD<%=5gF4c<lA><J<?Q<.R?S"
        "%-/Q#@<^;-WiE30g.(bH/Dv+Kj/w-$br^+>?1DR<RA?fX39MhLO:pk2]BF:.&cGg)m`0i)2^X2COZo6<rOt:C(23qL;A]/0pSaIhFY?DCP5T;->$u$%&WO;-BjTP&B[RS%RmrZ#(iNn0"
        "q3P,MRT8f3Y8mA#r8oQWXg^R/eJ0Pd#gL*6p)M*MM4N*MZNt1'-2O$Mq&T9C[PbIh3^Ps-L5k+MHH6uulhP;-`U18.>gQ#-%ua-?=7H,*jPe)*ow3c%=CAC#DOi8.'U&@#J29f30d%H)"
        "*t%s--sxs$7)(=6@rFq&6l<d;2d8T#eGv+RrkU/$NYwi>pUHh,^26O07+20C1#kjLC^GrHoV_'R;FG&#,0,t-#L$iL<N]L(LeCH-#7w&/C7r?#41X<8W<orsneKD?A<B30+-2QC)Aa%$"
        "St005lZxF-MaI6K6?TrJV<2.I#S.3VB8xiL7Tn##a^''#I`gT7IS($-cEk9%*oNn0>0;hLs,Jd))afF4EZ$@'vKCo0o+`^#D.Cv$X>a(66hQ;-XV>=lFcof)f$Y=Yo8Yg$LiY,Kh=+9/"
        "DX=r/e0P[$snP;-f.vS%mH-Zt<E&;QK.xH,.n4&7tS2q/c=-a5)[Is&A/#2C'7O2CI+M3M_uT2^SJx/&>ImO(.@cS#GJ(p#Enk$/uw(##TQj-$cehZfWq;?#UZZ;?n_oF.Roqr$W`qP8"
        "Q@dV[Q]Th#2B0[M^eDJ'$Gb;?pFqB#W@.@#$'CJ#HSGJ?e]O&#b=Pk%Wg@?$*;YY#S1U;0e%l0M+b;;?Vs9:2rvKLNOUZ##EtTs'1-;hL6aA@#`r(kkv9L(ARc`:2mLm]ODknO($o%2'"
        ";6jl?&G4Aucr?T-d$EE%4ctM(Ev>X$;FV@.PV?B=b/'Z-cpfZ0]Xj=.1(^S#FBlo>q/#+%/6B2#T++,MkeP;-:ar;?EdkfGUJK_%/XSHc91lQMS'wIU@WK`apr.;6;hV/2;s,t$w]B.*"
        "]-'u$Un0a#Mq?jBP2QK)4(7]6gH7lL$KMG)sN_hLREfX-u3YD#aUp+M=)/)*HH3$6puxl1ULY>#=<a*cOrcB,$_I70_2n+6$N&+QOg?O$Tt5;dKJDW-8biV-=xG>,jBhb=>?l132=hf)"
        "'RMx(q@K60,YpBX;s13aJ:?>#gxqs--d0)<J#tY-CgrA#/a3b$M6Ts-?\?<jLP.1Z-OYC]&6_Aj0(_JJ.oNv)4V%%aED-PEt?JxCu?W;3CetKJ)s>W]$h)*e)adwKP$.5nWHgsKPm7m=G"
        "sOY-QUt'3$AB(7#,Fil%BY6W-j9uBA#f+D#0sqYXG-Xpf%SN/Tc#Pv7CU`UI%k9xLI%###wF5s-e/O$MrQrhLhn(-M3uID*ov@+4Nf,lL$k3Q/UP5W-jkx<(v:+5A7uMvL/xocMa:2G`"
        "-DR'OK?CU9o][A,%fQ2(4b/O(0Zc8/HjE.3%q]8%MM?O(3dO;-SuMD?=[o[u.26(&io3=?N_W;-=KeA?:3js%#5L^uLJ76/%3uM'OKIIMBt;fUld%_]U8JwB8@bqg?4aY$+%^=7Kwr?#"
        "0S<+3MPsD#+IA8%J#ID*,+:^#Ka`6N_j5D#PRC16bR2o.W;LpA&H$9%KlKAt%1@A,+lbOoi0%f3hgv;Hq9<Cm+90)*X*3&6q/Qn.XLi5B'EqS%I>XDs#)LA,/0_LpO6*f3W4OrLMhgKm"
        "/2`$#>@<9..Sl##/It/1jH3Q/fU:8.&N1Dd/NYS._J@5$Ii5<-+2)2'$RCG)lNEM0%'%<&A+Cf)EcG##+crB#%1]5/?29f3^*6,MgTdP.e4B>,7u>X(>&<+3G@oT%5H8f3k?jv6,:tG+"
        "o;An1//DH?Rk)d#S>1?dn3A+`O#N$.+4J30c#HMXRpRQ0`o=8CQr,[H7uw97%3)6:wAi$>'vus-fR(dOg.:a<mUve43w?AOMgSV6`xn<:](&8q9F7.uASS;-W<ZGRJnu?CPn+2=[KA(H"
        "2lMY$>tqFrSO9MB:s79&b7m3'h`)fO=2:Z-F9B+%RS/H2oW&B)%RM1)6kjNL]PMP0^SY%&dfv1'GZWCj$kuY>fx;[$Hm$)*L4:$)G.Vv#n6+T.-E?A4)T9s$4fMB#?3Gg19[oC#^RS[u"
        "4rpl/Zw2mQi;8/(LrsGhfhQ;-D7_J#11F7)tZp(fW?A3g2IK9JCao3%C;Dk#ID(7#UEaP]&(;]/2[9&$,CTm/_R(f).73Q/W:`e$@DUv-?]d8/[Xxw#iN%;Q2=hf)t_6$6[1+51[(J80"
        "lL&s$X#c]#bcHs-I;:-<h)r50F@dY,q@M<-j6We$Z,g;-6-0b%NRg(Q%G%C#Jr0[%-'k=GYS`m$Ygh>-(7,h%6ImO(4`%b/XRG)4:AK?dU]Op@P@Wt(29:N0XDF&#D-4&#@Y:l95Y^-6"
        ".p[D*MkJv$&O(02qNh'HVIR8/OM>c4o6T9Ci#:9AM68p3/bc;/wwpouVuP$7U2t(G<h4(-91`'Rlg22D(q5(-t:'`QM7`P=+WvNFLNPN09@%%#Q,Gj^..GgL?7^J1<5'eZ1Ar`FXq*,4"
        "IjTv-`aoI*?/=>d$tn=GR)Bq%.sV=Yp/R['?T3>dw]I7nr2</&fdc;d+J*F@-__$Bpi5<-92SB.;%'h(]:nW-vP8-u>)0i)<W9.$nj@e#RS`du%k=DC$fB^#QgINMEaIa$J),##^9)8$"
        "U<_+mic.)*W+]]4[*QZ%JRo8%Tc:Z#Ei?<.V,ZA#]%-29Es.[53B'EY3d$=/Xac_=V8w@HP/,j':Vh3?;a5WRYG4=CG_(T:IZ1aue)q%[6k:d2+5t9Dwfx9D1[sr$N.2KEKPB.*[hi?#"
        "#,NF357[x6[bZN))c7C#S'Ig)dk=X(pu8f31m@d)nSID*-%1N(5Llj1(7U`>#f[w#*CAu$l%W4'AWio0lV?;%*HW8$rN$ENF%mW-1h;N,fW2^#Soq68;vv$v`=At#a(U'#fdS2^7XW$#"
        ";Z[0)1RL9Mc%g<$D.Y)4#,]]4(R7f3_Jp5AYFn8%CP,G4L#G:.l@h&6]?wG*^Tbp%Pp$[-YJ.d)@LR8%gTl&>bdh:%Zj::86k@e#S0P%?ZhN_,$(Q,*@:Z(+ZhEC,U;rG)cxN**5'A*+"
        "ZfrL1G9=:8ej_<9w5-%-H?[i96V22'dA4P;Ar3X.oCt$%4dChN,ZGg1AG+jLs:Rv$0DHb%/W8f3w<uD#.B8$&wLU*N`$o>QO(g^$/N]O(*X/A,7PGk<Lw(A5IrdM=&Qg1:>l*B,91Jh_"
        "1rtA#n-[F69bi=-)Y8#%%9ve<EHDr&YoBj2-BHM;`ATcGLVA>,mE&#*;a%=d]d`oA&a:%I-rY<-YwhQ8mAT;.>v:Q/xrZ&4nQXA#mJvJ1'LA8%55%&4iCAC#o4/B7@N<$6GM1Ddh10H["
        "&..Pd1MBDXp&r3C*cF1$:40@-C3A/2e:88SWq,IfiBj*%Isn<a<BsYad+m8])0KfL2=LP8ew^#$J$ok9NHSj0`N<P(p@7<.vu/+*T5Du$IU13(8E%8W;]dIh4hC#K+.VU?.MU7XtuW;-"
        "9id;@U&@#MDvEW#:,Wl=wC*-?7huM(.1wX-MxG>#Ak<PXnK[h#OUvcMK''P#vUJM&#ITa+:.OF3)DYo.ZcRUdb*(m8cKW9`%0S;-[PE`N<dic)Dqa?#aZ'r.gUMW-<SEL5:)ap^]q)$#"
        "6o/k$/k#R&Xw[J1@SW@$.kE:.K?#<.BRKfLIVOjLrND-*ThWO#leFHdT+U#X7uj>dodG%I9mqlKtE`?W(H0IV0:(Lf+hB'IWR#S&#C(,)b[18.%'%<&5ckI)A]G##=Z[0)<L:_ALR^fL"
        "fEPA#d?8d3W6[UT..;3a27wBC(DeLJ(8;=C7@Ga[$Cw:0Gu8Z$&feCa2*n_VTF^*nL'sGCo^5`HJ-(k&9D@j0SEaP]ckIJ1M]J,3x/_5/9`p>,VCs?>/>uo1NXFb329$C#mS=`uM*cB,"
        "oWG80=W3$6DC5-2hX6#6lrfs)n$wBt[_:$624L^#XX=)#o_d;-L=2e$'a8>,PkVT+BSYY#39Ks-5seC>h/>)47?v20BsZ&4vBo8%6VA1;`I,;Q'_>]u.ob318q.PdYkZ:([v'k#Ohv1'"
        "Fd<U)8:@5/U'xU.c_$<SHQ6=&&'grKV$WZ#fS6g3BgdC#D,_^usT6^gWNGFdt=(nu?+hOdteMGZh$,qKfW/%#/S2I.ZXM?#*gJD#6hX=$kpB:%X:Ls-i.<9/g]7Y.c;R)*qO`S%FpD9)"
        "kmTM')>wY7el.[7?R:F#C(ZCj^0v.3r^>r%xkkf,8>b:=AW[128wZ7-eaCo@OWe>^DKd#-sOQ-H7q/W-,hrR8[,lA#G[+Y$)?PF%Q0'#A[/_#$0CpFCUxT<hh&eA#(JU5'$fTB#agfI*"
        "66;hLQm`a4=dT3C_Z3>dg2N#$Df(T.3tuFrpD5T'd:35&CEcf(%'%<&[WpQ&Nhi2M@mnO(cFGH3L#G:.#&%Q/1r7DCUeRUdRY0[K20Biu`4+Aux$jrF;,$eko_g1<++pE@#DSq;;S?>d"
        "DA-D-FdC>8lpBmVREIU7IKh;I@^2<%-xNn0,Mp+'$,-J*)vsI3>I9]$,'30;PcD218NY7I_iq&5(p(;QX4;f$00O$M$R>L8k-=2C:L3>dw8_+$R8La<6w9B#r]i,)Lx$m/:j8>CZkTIC"
        "o:gV-ks*6L5[.Pdo_A8#.+SaExR?T.jd0'#4pm(#T%T*#(pkn8bVTf3w4eW.6itI)KF.<$/(On0xJ0s$-Q'<-)noH;HBIT*#7=.-F3OZ0O)cB,VR,W-lS#+%)[txW+=2.Itn@&FBD4Au"
        "qCxfM@fP;-tNpA,tC`e$$jE_&uh1vH?JreMiMrtOo3NK:Som_#XDF&#E@%%#S,Gj^pVr0(.j^s$uS52=34pL($A(s-R&SF4#':Z'WO2?u^GwM0GbN?k(C.>6:0niLPJ=jLiK/[#oxBI;"
        "9rS,3:wd3`g-I##BdS2^4D>##9H%O(03HA47FUhL&5oC#D6&tu&VZTIcm'^uY+'U#3CU9'Gs0&=S8-?\?1435&[,mQWh0bT%bvD#$@@.H3Q7qo.GXuS/L-YG%<j5/1wBw]-LW1F%a,bfu"
        "jn4[$rRcIhWQO>d:ThxM>rmh'*,pu,%'%<&eWAW-]VT49m:ge?M$nDNLt2.(Aa2.IRjjrq@)#)>/Gj.(Vf9vHuDPn*w9m1<BC$_#>[u##EdS2^wVvw$t4e;%VRG)48'lA#DX_a4%#q3C"
        "]DwFrwxxa#_Em=G8PL6MLnx,VR#O=(F`Aq^c93$#IL7%#`bkh(Z`#&)&*nD3BnPu%F*kp%^/p5S'9]9:KO1/1U)6<-+].:.tW`-#6,NH2KEaP]_kIJ1@16g)w*gm0dG`9&wgx9..FE:."
        "_r0%6kUlf)L6#53kw*B0_CQ;-NKxCug1uV$T$n#6PEpOMB[vY#'`87.qjQiLX5P=$Tr[fL<knO(W=K,3he9:@bb5g)-24m$=FS_#MGY2CZ.15A,m7I,Vqd[,;O&7/=,xr6Vs$21QMp=G"
        "(6B]uOTM@,d*,5T$v`K(tj;TD38t9D/9s9D,D5;-OG[#-.Y]fLB?@A4;'PA#YiWI)N_h&]cZs?#)CuM(xQ:a#2aTb%@rJ>(%seh(5qUJ)v>^c30C%X-cl^',K<Y`uak@e#8Z;v#Qts3T"
        "aCOC,/U]w:sH&;Q+^>[J6_r=G:1PG-[Hof$9REM0`:t;.2VJfLG7SX-1(:+*]R?tJec]+4wVp.*4+h$[N_j=.(0rC$G,^w6%4^S#QFD.*[h3LDVCi2Rva)re&YiPZSMuFrt^#l'**Lq%"
        "S>4G`HYET%41*I-1O/72D1IE5704mQ-Xuw#@awX)0@lm'L?^<8mIgZf^gO9%)O$]%I>mA#2K4L#5m;;?[iZ=YBul=G$jV=YA<pHS,S`du%8V9CsdVU%OYl;-jf[m-XaI_8l]B.*.eBN0"
        "oPVD3;S<+3*akBO<5:Z-tUXEeq645Cu3`19'gM*6o(PP0F7I20%RM1)UZC80bmOh#]PbIhL?R@,`f*5AIuN#-U=*OWaEj$#Xq$*..C(-M]-'CO3@Bj0LNv)4]MUM73vOG=6UfM=`ZW>C"
        "$ed19avj=GAWD80$tn=G`9#j=w[)_f-u13Vm^3$#^Kb&#B`gT7?v+B,^wnW$*oNn0]K)%%Zd*P(d+8`&3,7`&5ptNFd/<-&vX0^#0W5W-6q3+%TEKNrWtvYuT(;?#HYYF%mZ&ZulT7U."
        "a0j=GTu_H-'NA[9j=%a+0QSn0ji),)$S'41TH8&,2xg%%_c^-Q2i:8.AKwUMe8nqMST3F.?FYI)Q#P)4XIVp.`8i;dd*3I$PUSCMbV0K-Ag=,/xZ6/&n=ge$f>s-&/>i,MfER$&V)f;d"
        "'41@'_imp%Pcd/:q;h^#87>##9hVr7Fa,[-njTq%-M:;$eN;hLq[3r725Ld4ukP;-n-cp0,s$&4oLc'&*rB^#?PH$8^+Na,AX,hL(-[pgPGE,#PSuD%`=K,39sZ]4]52T/&@Rq$rUFb3"
        ",asFr<o.qL0whf)a$M@,cBYG,2>bduvuMvLjmS@#+.,59+SJ=%dhLS._,$h.^LVZ#)lNn0-%.+5V`v[-kT&],Enn8%O=Y)4wHuD#v.<9/tgOU%U`G>#'6h/N7l5/1PcvsK*=4s-hN<r7"
        "'1Xg*L]rP0UXtUI7`d4Cd>O'gL/m/C2$).P`YP/CTCRr8RMB58uVZ?\?Mn78%:CRS%366Q8]`O_J>,guC(pH@9SZK<-Oe/;%>H&7WT4T&#HYTd'&fB9rp`UWf<;DD3/?ID*;^9Z-f(]]4"
        "s@&d)hJu-$6LhF=V765/.?Gu>V465/F`R9C-1niLpXn'=D(`4)-U%Zu33GouI(*kNmv2*M$tv##'X6iL%pO.)jH_Lj+wn5/Ak3Q/`W]I*=sG,*Z96a3mtIpYRxed]@86M?DrZ4:Tw/I4"
        "(cKs$l=i@@=R[euf5*523C)-::KF&#qt)W-1;b9D5DPV-T9om,NcYY#TCTfL<,OF3*wRa$)+&s$rTZd3b@u)4+Rw.:XJa`#0LQ3B9978R9j._u?5?,,,i:8..pK+`Y+Xj$::h0,CjTJD"
        "P*U@k$1P]>;>WJ2Gk<MuPQvt$N&###8&M/(N+FD*pk8K1;F9a#AM>Z,r2Gg1Y5#kkf^#;/+gOxtq6:].3fWiCR;dc.J>V@uP<[:t9KCR#Gv8h=Ed[x+:[K#Qw3l-,SUY$dp2QD-*/Zp%"
        "7L[Q''fNn0RsMhL(mUD34WFL#jL3]-``1I3(Ci;nJj+ru$,_^u=W/+%JTW;-9Ib2C(q0Pd[Ln;-`b(g@`CrT/am45&O[l^,tGn,Z.s_A*[L2*m%],G4$L[+4p/0i)NIHg))wZ)4Gt%s-"
        "*^JW$<Utn%V:.%@J@?Yc,RhVD;>tK(m-2hPZwO3C#-grec;uC$K8wp@aST/D420kL2Hf'R/ogJ*v:P^?f,Q_/P%5p@Qk(T/uxmQWl/]VW,c5g)QO*e.k*_^uV9RH.jZA>#ma;C#aOk9&"
        "I5ts'alVP&/.=X(,3cv*0NcT.m=V0$V_`=-Wwnf$P####'>uu#4#Z3#v6QtLUOvu#AbW3M1ilS.&?F&#D+2t-[.JfLf:qV-6680-XEM_&5xkA#R&+>.F.$##ejZF%Bk60Hi^u-$&m_#C"
        "NpaR3U)'#_rvg=MHu)?7]f`;Y8NkfMB8O?R4IsJ%JImQWHqPF%5lQ78Kqe`];>%RP@q44N$en]-3I^l4[1ivPx:K-MF1:x&mp`gL)7GcMZaA%#r&3aaaBV,#4e53#wDmV#*OGF#fejL#"
        "Q$bT#*1rZ#,8W`#/B=e#0@gi#5Iuu#JBr($iT<5$&IR<$ULaH$jx%T$Vl;[$uAXa$Lvc3%R)=&%*TT:%tJ%H%bCUn%Qj,V%+XAa%+%wg%8/]l%<<Bq%qrp#&HjuJ&8B[<&Bc9F&bGgQ&"
        "VZ3X&('i_&H#dn&KLSx&]ieH'NXD<'ZogB'K*<h'/?(Z')1j`'l)p.(qY,r',Knw'XP[D(,6*5(fW_;(5n+B(D9_N(:&tX(S*a))vB$k(Jo>v(>+^2)([g^)VflN)@+'#*l*qf)5@=m)"
        "B7B>*$h)-*d235*[:B>*q^vG*Wv#c*i62<+efC,+8cl3+(FD<+=u_G+uFNT+XExX+Z_p^+e(%d+hp;h+um9n+Lxsx+A3?,,9Ss5,wv%A,N$4M,[rds,meEa,PaBj,h4<9-ObL,-csjA-"
        ")#NL-N^'R-uibY-T#-g-dfv4.oKp#.RTT+.pd(S.h.QG.f(Wl.o>D[.#//e..qL4/rmj&/;fDL/e>V</6]2L/Ys(W/L$9^/M48.0'W7t/RMgF0/#####YlS.oC+.#2M#<-MeR+%h)'##"
        "&/YaE0]Wh#WtLrZ[dM_&dQd(N/(1B#SdrIh01L^#iq<on1:h#$-MWY52C-?$KTfuG3LHZ$##QiT:$Ew$G%w@b'5>>#]F[0>M3o-$.(1B#L[+Vd<UL^#aYkCj1:h#$]xR.q2C-?$@fEPA"
        "3LHZ$i=w1K><jw$:n>YY.JCYG?mv1BujNe$#t7;-T<XoIu5Ad<FKWf:D49?-6J,AF8k)F.D/Wq)&APcDU^UJDk$cP9GWMDF;F5F%uuo+D#ql>-]tn92gdMk++I2GDYo'?H#-QY5+v4L#"
        "[Z'x0_6qw0>j^uG9T9X1lrIk4+-E/#:,BP8&dCEHp4`PB;^5o1a:XG<2/_oDCt7FHbmndFgkqdF-4%F-)G@@-G3FGH,C%12AMM=-H1F7/Y1Vq1CTnrL'Ch*#D):@/5d:J:<N7rL9et3N"
        "5Y>W-xrv9)iaV-#Cv#O-iBGO-aqdT-$GqS-'?Z-.;9^kLJ*xU.,3h'#WG5s-^2TkLcU$lLp6mp3Ai0JF7DmlE>FK@-1CUO1?6[L28V7ZQpRC`Nn4$##+0A'.6sarLNf=rL]3oiLPVZ-N"
        ">2oiL?Z/eG38vLF%fCkL-:Mt-0aErLc_4rL0)Uk.,gpKF,r0o-T?*1#u<*1#rENvPWpT(MT)rR#(AcY#,VC;$0o$s$41[S%8I<5&tU^l8Ym@M9^/x.:$sul&@$TM'D<5/(HTlf(LmLG)"
        "P/.)*TGe`*X`EA+]x&#,a:^Y,eR>;-ikur-m-VS.qE75/u^nl/#wNM0'90/1+Qgf1/jGG23,))37D``3;]@A4?uwx4C7XY5GO9;6Khpr6O*QS7SB258WZil8[sIM9`5+/:dMbf:hfBG;"
        "l($)<p@Z`<tX;A=xqrx=&4SY>*L4;?.ekr?2'LS@6?-5A:WdlA>pDMBB2&/CFJ]fCJc=GDN%u(ER=U`EVU6AFZnmxF_0NYGcH/;HgafrHk#GSIo;(5JsS_lJwl?MK%/w.L)GWfL-`8GM"
        "1xo(N5:P`N9R1AO=khxOA-IYPEE*;QI^arQMvASRQ8#5SUPYlSYi:MT^+r.UbCRfUf[3GVjtj(Wn6K`WrN,AXvgcxXot*GV&6`uY*N@VZ.gw7[2)Xo[6A9P]:Yp1^>rPi^B42J_FLi+`"
        "JeIc`N'+DaR?b%bVWB]bZp#>c_2ZuccJ;Vdgcr7ek%Soeo=4PfsUk1gwnKig%1-Jh)Id+i-bDciu&FS()Foc2cblh25Lx_#f/mc2k0.+49eF`#j;mc2sTEC5eh#e#?inc2t/-4Ci*He#"
        "D(4)3dhuh26Ox_#h82)3l67+4:hF`#lD2)3tZNC5fk#e#Ar3)3u564Cj-He#F1OD3en(i27Rx_#jAMD3m<@+4;kF`#nMMD3uaWC5gn#e#C%OD3v;?4Ck0He#H:k`3ft1i28Ux_#lJi`3"
        "nBI+4<nF`#pVi`3vgaC5hq#e#E.k`3wAH4Cl3He#JC0&4g$;i29Xx_#nS.&4oHR+4=qF`#r`.&4wmjC5it#e#G70&4xGQ4Cm6He#LLKA4h*Di2:[x_#p]IA4pN[+4>tF`#tiIA4xssC5"
        "jw#e#I@KA4#NZ4Cn9He#NUg]4i0Mi2;_x_#rfe]4qTe+4?wF`#vre]4#$'D5k$$e#KIg]4$Td4Co<He#P_,#5j6Vi2<bx_#to*#5rZn+4@$G`#x%+#5$*0D5l'$e#MR,#5%Zm4Cp?He#"
        "RhG>5k<`i2=ex_#vxE>5saw+4A'G`#$/F>5%09D5m*$e#O[G>5&av4CqBHe#&'32B=.wm2f2$`#q712BER804jJH`#uC12BMwOH5?N%e#Jq22BNQ79CDmRe#p7LMB@=E33h;-`#tCLMB"
        "Hb]K4lSQ`#xOLMBx;D<BAW.e#M'NMBQa[TCEpRe#r@hiBACN33i>-`#vLhiBIhfK4mVQ`#$YhiB#BM<BBZ.e#O0jiBRgeTCFsRe#tI-/CBIW33jA-`#xU-/CJnoK4nYQ`#&c-/C$HV<B"
        "C^.e#Q9//CSmnTC$),##$)>>#hda1#mPh5#vTg5#pHf^?ht^6_#2S4;2ZR%@&g6c+aq-vA4rkL>S=*p.F.s##GM.tB:oHY-N]IV?:K=?.Ao.R<ZjfsAD0-Q#+^+pAj9Hm#IVoE-@/c[["
        "ZTC>]DlGQ8d*RH=q1a;.B'hI?\?C-i<8F4g7ioQV[b+MT..lbD-Lx'<.m7T)0KNI@'F]+Z2.HZV[#9U<.WKXD-1*6EO`ZwG&Y_bpLx;4/MAbl,M[kQ1MW6ErAQ.r`?IO,Q#$1_A.'$VC?"
        "i6oS1#Yp?-=$)N$hpjHQ<E[g<T=e8.?Cn8.$<*+09MglA13P/11>;E/_UOj$lM5/Mis]Z0'f:Z[$tOZ[IVxiLkEBB-0_3uL8;5Z2)[S4=Gs2f+3QUD?=5Ok+1KtC?Nx%a+t9_eH7TDj$"
        "Jk2Q8XO`Y#_0>?84I1gLq.h88mTg5#EmM^[I@'_[0c._J*'#N04El/18x7F/8o99_b`qpL7]2..KU=5Ma1tv8%N+X%jv>Q&C8H<8ng,ND,K#01`SY<-o*t^-TM@tLL_K?pewI59RNSE-"
        "7lS?-H-^V[uL+Z2@vv>-qZXv7rR)W[%)^5Bp&aE-$44^-Z,g34B3lV7G),^O[^>01hM8;]+o>290h';.>X&[-:jL01tQ5293Y)w8;pU01Z,[?-C3aQ8oaT`<*#DT.=<Z][6uY<-%sY<-"
        "7.;t-=*.hLR_vNB0G/?-C[SiB61BQ/0HesAN)NE-Gxf884cpA(Gs'D?,IAw8_,tD'QN1W1]^W0;l6Ne=Nr.a+SX`=-d[Y=6i)q5##a0D?6avQ8#rD?8h%wZ?sF:Z[>cMv[,>^;-0`uS."
        "d>N:.fIY`1)d7Q#xiD^[oV(HOtxa*I_P_;.oYs_&XZ*.-Wdi^#iB;a'J8voL,v&nLXJ4tAm`/aYx*RV[A)eU%Z:)##N`DZ[nih5#lLM>#`0>?8@8.11,L`Y#qmQ][c<9B-GbE9.C[<9."
        "wh;?8Ou*[K08o0(2xL$#=;B%BnYAj07v7b+'#vW[h6mh/?BtW[m26Q#<)d,*1=(,M;PM11c.5F%<b/t[*lu8.uVMj$@M%E+5UL,M?ir11g:5F%@n/t[.lu8.'&f,&F(O^,cjrD'T>gI-"
        "nWeI-mWeI-'-@:MC8xiLVQ#<-)-@:MED4jLVQ#<-+-@:MGPFjLVQ#<---@:MI]XjLVQ#<-/-@:MT'G311^-Q#6N3a0KJco.LeiS8PF%U.>=*p.Cb_&4Ht$[-Xt^31/5bW[U?tW[g5Y>-"
        ":E%HF:/^5B)/HDO9[4h-3bqY?p=Vt7/ChB#@h0R0pXQI2Ok5R*sIZj)NQx:.)I?)4J;gJ2@_lA#lL$^(xmoi0WLq-MtS%)*[;w0#ENh>.=`EC#O5`P-Qmm*MmfC12x@HP8EktW[r89I*"
        "qI(W3lYnV[M0'qLCKV41BG$)*bf-@->u68.==*p.t2Gx9h:_5B+ktpLK&-Q#o?;W3NYWI),p,*4lS*)**SEjLLYDm0KYmp0em-=.*$GGOG[[D4bhb>-nQ=v-%=ufL]hev6ONjj<7C4g7"
        "-#fm#,7%a+W+$$$qi-<8uG@g%>^3uLVPq%/HsD^[*8WW/>+KE-vLg?-e0WE-$$esA_$d2$'#6k<+vlW--?D_&]A[][djPS3e,K*R&RW5/&;7-5Eai?B9KJ9io/vERAcM>CQ@#B#sg([K"
        ">0]3FwNEp.DxvuP$.0oLGD7p._FOe$E50HbJ3jU1<o?p4lS*)*4(RV[=9q&$X/o>6rwZ55<smV[i^NmLumY?.s28b-A)8R*ma?R*X#<s.t+SW[S)+p1o+;Z->R/N-AqYhL,?(&&GWEjL"
        "nkHq9ohN)Y-Z9T(_eMs7]:^;.*dKK-0:%I-S;m)/u7%N-]_Bil[TXp.#Hu>-*D,XCBWkC?]<3p1kbMG)iEsM62Qsc<xOnY6J=rh55CXv6aQHo0$s%=7bWQo0*fR>6=Yf88F&eG.@XmP8"
        "Z(-kLE]o;8+dUv@[V[,&cHV].&,*=.V=SF/JsIg%&w+<8LV'HOK0Y<8hbxV[GQF9B&0:?6fDGY-?)1pprsqY?_?u88aQHo05nw;-a2gr-Skt)#U7[uNRdamL9dWR8O8S9]:?hc))U&X["
        "]?tW[>3:o/G*q5#Q`0U[tgt;.&PCs3j-C2:ebE-*/D+j17uqF.IiF<%$BlW[AS2Z[uBtW[L0$6#D@Ib%o_Ib%%0Pb%r]L9]/:)=-*2#HM%HgnLK1(588pWG<$]bA#u&sc<U)OX(uqIb%"
        "+TSD=6:P&#'Lx>-wS,<-%V,<-CIrIM,sPoL6:)=-7u@Y-_1(F.%.Jb%%0Pb%8)4O+2>hY?CHNX(:/4O+`OVmLDA2pL%+p+M*G;pLH:)=-:2#HM5SMpLHl+87HUaSA$]bA#/c%pAU)OX("
        "/LJb%>LxPB#%]u7k'xlB/HZV[2KL@-5a5<-e(MT.f$h5#UT=Z-JZa$'4DmW[1E(@-IMGHM^L4RMa0nlLQR=RMR>_<.N(wP5b,RGEb6L[0=-#(&_>3)Fd(nY6lxjDFndXfNanX7MS&c?-"
        "Aa5<-x9]>-P5g,Mg&5;1;k(Z#tco;6Ij@g%a:nS.Hi/Q#@oi'#Ebl:d+d7Q#hw]][rwB`$nLs5#rlq5#/YHF%4s1G%VR)##mINX(=3<%$=cDg.VndC?J7VI?+FAw86$/@':bc/Li+kS7"
        ";8JoLHF_l8.V&,2a>]N0c3X(NmpL(]jh8*#1J>lLR[+##";

    return openIconic_compressed_data_base85;
}
// clang-format on


void ImGuiH::showDemoIcons()
{
  // clang-format off
   static const char* text_icon_[] = {
    "account_login", "account_logout", "action_redo", "action_undo", "align_center",
    "align_left","align_right","aperture","arrow_bottom","arrow_circle_bottom","arrow_circle_left",
    "arrow_circle_right","arrow_circle_top","arrow_left","arrow_right","arrow_thick_bottom","arrow_thick_left",
    "arrow_thick_right","arrow_thick_top","arrow_top","audio","audio_spectrum","badge","ban","bar_chart","basket",
    "battery_empty","battery_full","beaker","bell","bluetooth","bold","bolt","book","bookmark","box","briefcase",
    "british_pound","browser","brush","bug","bullhorn","calculator","calendar","camera_slr","caret_bottom",
    "caret_left","caret_right","caret_top","cart","chat","check","chevron_bottom","chevron_left","chevron_right",
    "chevron_top","circle_check","circle_x","clipboard","clock","cloud","cloud_download","cloud_upload","cloudy",
    "code","cog","collapse_down","collapse_left","collapse_right","collapse_up","command","comment_square",
    "compass","contrast","copywriting","credit_card","crop","dashboard","data_transfer_download","data_transfer_upload",
    "delete","dial","document","dollar","double_quote_sans_left","double_quote_sans_right","double_quote_serif_left",
    "double_quote_serif_right","droplet","eject","elevator","ellipses","envelope_closed","envelope_open","euro",
    "excerpt", "expend_down", "expend_left", "expend_right", "expend_up", "external_link", "eye", "eyedropper", "file", "fire", "flag", "flash",
    "folder", "fork", "fullscreen_enter", "fullscreen_exit", "globe", "graph", "grid_four_up", "grid_three_up", "grid_two_up", "hard_drive", "header",
    "headphones", "heart", "home", "image", "inbox", "infinity", "info", "italic", "justify_center", "justify_left", "justify_right",
    "key", "laptop", "layers", "lightbulb", "link_broken", "link_intact", "list", "list_rich", "location", "lock_locked", "lock_unlocked", "loop_circular",
    "loop_square", "loop", "magnifying_glass",
    "map", "map_marquer", "media_pause", "media_play", "media_record", "media_skip_backward", "media_skip_forward", "media_step_backward", "media_step_forward",
    "media_stop", "medical_cross", "menu", "microphone", "minus", "monitor", "moon", "move", "musical_note", "paperclip",
    "pencil", "people", "person", "phone", "pie_chart", "pin", "play_circle", "plus", "power_standby", "print", "project", "pulse", "puzzle_piece",
    "question_mark", "rain", "random", "reload", "resize_both", "resize_height",
    "resize_width", "rss", "rss_alt", "script", "share", "share_boxed", "shield", "signal", "signpost", "sort_ascending", "sort_descending", "spreadsheet",
    "star", "sun", "tablet", "tag", "tags", "target", "task", "terminal",
    "text", "thumb_down", "thumb_up", "timer", "transfer", "trash", "underline", "vertical_align_bottom", "vertical_align_center", "vertical_align_top", "video",
    "volume_high", "volume_low", "volume_off", "warning", "wifi", "wrench", "x", "yen", "zoom_in", "zoom_out"
    };
  // clang-format on

  ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);
  if(!ImGui::Begin("Icons"))
  {
    ImGui::End();
    return;
  }

  // From 0xE000 to 0xE0DF
  for(int i = 0; i < 223; i++)
  {
    std::string utf8String;
    int         codePoint = i + 0xE000;
    utf8String += static_cast<char>(0xE0 | (codePoint >> 12));
    utf8String += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
    utf8String += static_cast<char>(0x80 | (codePoint & 0x3F));

    ImGui::PushFont(ImGuiH::getIconicFont());
    ImGui::Text("%s", utf8String.c_str());  // Show icon
    if(((i + 1) % 20) != 0)
      ImGui::SameLine();
    ImGui::PopFont();
    ImGui::SetItemTooltip("%s", text_icon_[i]);
  }
  ImGui::End();
}
